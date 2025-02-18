#include "event_loop.hpp"
#include <iostream>
#include <thread>
#include <functional>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <string.h>
#include "log.h"
#include "epoll_dispatcher.hpp"

Event_loop::Event_loop(std::string thread_name)
{
    is_quit = false;
    // 多态，绑定传入的dispatcher
    this->dispatcher = new Epoll_dispatcher(thread_name);

    // 线程id通过cpp的库获取
    thread_id = std::this_thread::get_id();
    this->thread_name = (thread_name.empty() ? "thread-main" : thread_name);
    // 初始化互斥锁
    // cpp中的互斥锁不需要初始化
    // pthread_mutex_init(&(mutex), NULL);

    // 创建sock pair
    int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, socket_pair);
    if (ret == -1)
        LOG_ERROR("event_loop_create socketpair");

    // 创建了socket_pair后，将需要对这个fd进行监听，所以我们需要构造一个channel，这里不需要传入销毁的回调，因为sockpair需要一直存在用于唤醒，直到服务器关闭
    // Channel *channel = new Channel(socket_pair[1], static_cast<int>(CHANNEL_EVENT::READ),
    //                                socket_pair_read, NULL, NULL,  static_cast<void *>(this));
    std::function<int(void *)> func_ptr = std::bind(&Event_loop::socket_pair_read, this);
    // std::function<int(void *)> func_ptr = [this](void *args)
    // {
    //     return this->socket_pair_read(args);
    // };
    Channel *channel = new Channel(socket_pair[1], CHANNEL_EVENT::READ, func_ptr, nullptr, nullptr, this);

    // 将这个channel添加到任务队列中，才能监听
    add_task(channel, CHANNEL_TASK_TYPE::ADD);

    // LOG_DEBUG("create event loop successfully");
}

Event_loop::~Event_loop()
{
}

int Event_loop::run()
{
    // LOG_DEBUG("%s event loop running", thread_name.data());
    // 比较线程id是否相等，如果不等就是错误
    if (thread_id != std::this_thread::get_id())
    {
        LOG_ERROR("thread_id of event loop is not equals to pthread_self()");
        return -1;
    }

    while (!is_quit)
    {
        dispatcher->dispatch(this);
        // 如果是阻塞在上一行，那么在有channel_task的时候，会被event_loop_wakeup_event唤醒，执行管理的操作
        manage_tasks();
    }
    return 0;
}

void Event_loop::add_task(Channel *channel, CHANNEL_TASK_TYPE task_type)
{
    mutex.lock();
    // if (!mutex.try_lock())
    //     std::cout << "Thread " << std::this_thread::get_id() << " failed to acquire the lock.\n";
    // else
    //     std::cout << "Thread " << std::this_thread::get_id() << " got the lock.\n";

    Channel_task *channel_task = new Channel_task();
    channel_task->channel = channel;
    channel_task->task_type = task_type;
    task_queue.push(channel_task);
    mutex.unlock();

    /*
    以下结论基于一个前提：当前的eventloop的线程id时子线程！！！！！
    这个函数有可能被主线程和子线程触发
    但是通常来讲，我们的主线程只负责接收客户端的连接请求，不负责处理
    所以，如果是主线程执行到了这个函数，那么就唤醒子线程，让子线程去处理接下来的操作
    */
    if (thread_id == std::this_thread::get_id())
        manage_tasks(); // 处理动作
    else
        wakeup(); // 唤醒可能正在epoll阻塞的子线程

    // LOG_DEBUG("add event loop task > fd(%d), type(%d)", channel->fd, task_type);
}

void Event_loop::add_channel(Channel *channel)
{
    // 查找map如果不存在，才存入map
    if (channel_map.find(channel->get_fd()) == channel_map.end())
    {
        channel_map.insert(std::make_pair(channel->get_fd(), channel));
        LOG_DEBUG("event loop(%s) add channel(%d,%d)", thread_name.data(), channel->get_fd(), (int)(channel->get_events()))
        dispatcher->add(channel);
    }
}

void Event_loop::remove_channel(Channel *channel)
{
    // 如果不存在，返回
    if (channel_map.find(channel->get_fd()) == channel_map.end())
        return;

    // 先从epoll树中移除
    int ret = dispatcher->remove(channel);
    close(channel->get_fd());

    // 再取消channelmap中的映射
    channel_map.erase(channel->get_fd());

    // 执行channel的销毁回调
    if (channel->destroy_callback != NULL && channel->get_args() != NULL)
        channel->destroy_callback(const_cast<void *>(channel->get_args()));

    delete channel;
    channel = nullptr;
}

void Event_loop::modify_channel(Channel *channel)
{
    // 如果不存在，返回
    if (channel_map.find(channel->get_fd()) == channel_map.end())
        return;

    dispatcher->modify(channel);
}

void Event_loop::manage_tasks()
{
    while (!task_queue.empty())
    {
        mutex.lock();
        Channel_task *channel_task = task_queue.front();
        task_queue.pop();
        mutex.unlock();

        if (channel_task == NULL)
            break;

        if (channel_task->task_type == CHANNEL_TASK_TYPE::ADD)
            add_channel(channel_task->channel);
        else if (channel_task->task_type == CHANNEL_TASK_TYPE::REMOVE)
            remove_channel(channel_task->channel);
        else if (channel_task->task_type == CHANNEL_TASK_TYPE::MDOIFY)
            modify_channel(channel_task->channel);

        delete channel_task;
    }
}

int Event_loop::process_event(int fd, CHANNEL_EVENT type)
{
    if (fd < 0)
    {
        LOG_ERROR("event_loop_process_event fd illegal");
        return -1;
    }

    Channel *channel = channel_map[fd];

    // 根据传入的type判断执行读还是写回调
    if (type == CHANNEL_EVENT::READ)
        channel->read_callback(const_cast<void *>(channel->get_args()));

    if (type == CHANNEL_EVENT::WRITE)
        channel->write_callback(const_cast<void *>(channel->get_args()));

    if (type == CHANNEL_EVENT::DESTROY)
        remove_channel(channel);

    return 0;
}

void Event_loop::wakeup()
{
    LOG_DEBUG("event loop(%s) wake up by fd(%d)", thread_name.data(), socket_pair[0]);
    const char *msg = "wakeup signal";
    write(socket_pair[0], msg, strlen(msg));
}

int Event_loop::socket_pair_read()
{
    // 这个函数的目的并不是读取数据，而是为了唤醒(解除阻塞)epoll而已，所以读取到了数据之后，什么都不用做
    char buf[64];
    read(socket_pair[1], buf, sizeof(buf));

    return 0;
}

void *threadfunc_event_loop_process_event(void *arg_event_data)
{
    Arg_event_data *event_data = (Arg_event_data *)arg_event_data;
    event_data->event_loop->process_event(event_data->fd, event_data->type);
    // 释放传入参数
    free(arg_event_data);
    return (void *)0;
}