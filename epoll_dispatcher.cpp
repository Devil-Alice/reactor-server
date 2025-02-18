#include "epoll_dispatcher.hpp"
#include "dispatcher.hpp"
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <stdlib.h>
#include "log.h"

Epoll_data::Epoll_data()
{
    epoll_fd = epoll_create(1);
    if (epoll_fd == -1)
        LOG_ERROR("epoll_create");
    epoll_events = new epoll_event[EPEVENTS_CAPACITY];
}

Epoll_data::~Epoll_data()
{
    delete[] epoll_events;
    close(epoll_fd);
}

Epoll_dispatcher::Epoll_dispatcher(std::string name) : Dispatcher(name)
{
}

Epoll_dispatcher::~Epoll_dispatcher()
{
}

int Epoll_dispatcher::add(Channel *channel)
{
    return control(channel, EPOLL_CTL_ADD);
}

int Epoll_dispatcher::remove(Channel *channel)
{
    return control(channel, EPOLL_CTL_DEL);
}

int Epoll_dispatcher::modify(Channel *channel)
{
    return control(channel, EPOLL_CTL_MOD);
}

int Epoll_dispatcher::dispatch(Event_loop *event_loop, int timeout_ms)
{
    int cnt = epoll_wait(epoll_data.epoll_fd, epoll_data.epoll_events, EPEVENTS_CAPACITY, timeout_ms);
    for (int i = 0; i < cnt; i++)
    {
        int events = epoll_data.epoll_events[i].events;
        int fd = epoll_data.epoll_events[i].data.fd;
        // 通过事件判断对方是否断开了链接，EPOLLERR表示文件描述符发生了错误，EPOLLHUP表示挂起，可能是对方关闭了连接，或者是通信出现了问题
        if (events & EPOLLERR || events & EPOLLHUP)
        {
            // todo: 目前还未实现和channel的关联，之后再完成这句代码
            LOG_DEBUG("epoll wait errot or hang up");
            event_loop->process_event(fd, CHANNEL_EVENT::DESTROY);
            continue;
        }

        // 读写操作同时进行，但是单次循环只进行一次读写操作
        pthread_t read_thread_id = -1;

        // 非常重要！！！！！！！！！！！！！！！！！
        //  有时epoll只检测到了写事件，但是还没检测到读事件，所以这里需要根据情况判断，如果可以单独执行写那么就可以继续执行
        // 如果不能写事件单独执行，例如本程序中的写事件，就会陷入死循环，并且eventloop会等待线程join，就导致本次请求永远无法响应，那么就需要

        if (!(events & EPOLLIN))
        {
            LOG_DEBUG("read event fd(%d) is not triggered, continue", fd);
            continue;
        }

        // 触发了读事件
        // if (events & EPOLLIN)
        // {
        // event_loop_process_event(event_loop, fd, READ);
        // 使用malloc申请内存，在线程函数中执行完毕销毁
        Arg_event_data *event_data_read = new Arg_event_data();
        event_data_read->event_loop = event_loop;
        event_data_read->fd = fd;
        event_data_read->type = CHANNEL_EVENT::READ;
        // LOG_DEBUG("%s diapatch read event > fd(%d)", event_loop->get_thread_name().data(), event_data_read->fd);
        // 这里另起一个线程
        int ret = pthread_create(&read_thread_id, NULL, threadfunc_event_loop_process_event, event_data_read);
        if (ret != 0)
            LOG_ERROR("pthread_create failed for read thread, error: %d", ret);
        // }

        // // 触发了写事件
        // if (events & EPOLLOUT)
        // {
        //     // event_loop_process_event(event_loop, fd, WRITE);
        //     arg_event_data_t *event_data_write = (arg_event_data_t *)malloc(sizeof(arg_event_data_t));
        //     event_data_write->event_loop = event_loop;
        //     event_data_write->fd = fd;
        //     event_data_write->type = WRITE;
        //     LOG_DEBUG("%s diapatch write event > fd(%d)", event_loop->thread_name, event_data_write->fd);
        //     ret = pthread_create(&write_thread_id, NULL, threadfunc_event_loop_process_event, event_data_write);
        //     if (ret != 0)
        //         LOG_ERROR("pthread_create failed for write thread, error: %d", ret);
        // }

        // 等待读写操作结束
        if (read_thread_id != -1)
            pthread_join(read_thread_id, NULL);
        // if (write_thread_id != -1)
        //     pthread_join(write_thread_id, NULL);

    }
    return 0;
}

int Epoll_dispatcher::control(Channel *channel, int operation)
{
    struct epoll_event epevent;
    epevent.events = EPOLLET;
    epevent.data.fd = channel->get_fd();

    if ((int)(channel->get_events() & CHANNEL_EVENT::WRITE))
        epevent.events |= EPOLLOUT;
    if ((int)(channel->get_events() & CHANNEL_EVENT::READ))
        epevent.events |= EPOLLIN;

    int ret = epoll_ctl(epoll_data.epoll_fd, operation, channel->get_fd(), &epevent);
    if (ret == -1)
        LOG_ERROR("epoll_ctl");
    return ret;
}
