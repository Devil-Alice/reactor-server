#include "event_loop.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/epoll.h>
#include <string.h>

event_loop_t *event_loop_create_main(void)
{
    return event_loop_create(NULL);
}

event_loop_t *event_loop_create(char *thread_name)
{

    event_loop_t *event_loop = (event_loop_t *)malloc(sizeof(struct event_loop));
    event_loop->is_quit = false;
    // 将epoll的dispatcher赋值给eventloop
    event_loop->dispatcher = &epoll_dispatcher;
    // 使用dispatcher的init函数初始化data并获取
    event_loop->dispatcher_data = event_loop->dispatcher->init();

    // 初始化任务队列
    event_loop->task_queue = linked_list_create();

    // 初始化channel_map
    event_loop->channel_map = channel_map_create(128);

    event_loop->thread_id = pthread_self();
    event_loop->thread_name = thread_name == NULL ? "thread_main" : thread_name;
    pthread_mutex_init(&(event_loop->mutex), NULL);

    int ret = socketpair(AF_LOCAL, SOCK_STREAM, 0, event_loop->socket_pair);
    if (ret == -1)
    {
        free(event_loop);
        perror("event_loop_create socketpair");
        return NULL;
    }

    // 创建了socket_pair后，将需要对这个fd进行监听，所以我们需要构造一个channel
    channel_t *channel = channel_create(event_loop->socket_pair[1], CHANNEL_EVENT_READ, event_loop_socket_pair_read, NULL, event_loop);

    // 将这个channel添加到任务队列中，才能监听
    event_loop_add_task(event_loop, channel, CHANNEL_TASK_TYPE_ADD);
    return event_loop;
}

int event_loop_run(event_loop_t *event_loop)
{
    assert(event_loop != NULL);
    // 比较线程id是否相等，如果不等就是错误
    if (event_loop->thread_id != pthread_self())
    {
        printf("thread_id of event loop is not equals to pthread_self()");
        return -1;
    }
    dispatcher_t *dispatcher = event_loop->dispatcher;
    while (!(event_loop->is_quit))
    {
        dispatcher->dispatch(event_loop, 2000);
        // 如果是阻塞在上一行，那么在有channel_task的时候，会被event_loop_wakeup_event唤醒，执行管理的操作
        event_loop_manage_tasks(event_loop);
    }
    return 0;
}

int event_loop_add_task(event_loop_t *event_loop, channel_t *channel, int task_type)
{
    pthread_mutex_lock(&(event_loop->mutex));
    channel_task_t *channel_task = malloc(sizeof(channel_task_t));
    channel_task->channel = channel;
    channel_task->task_type = task_type;
    linked_list_push_tail(event_loop->task_queue, channel_task);
    pthread_mutex_unlock(&(event_loop->mutex));

    /*
    以下结论基于一个前提：当前的eventloop的线程id时子线程！！！！！
    这个函数有可能被主线程和子线程触发
    但是通常来讲，我们的主线程只负责接收客户端的连接请求，不负责处理
    所以，如果是主线程执行到了这个函数，那么就唤醒子线程，让子线程去处理接下来的操作
    */

    if (event_loop->thread_id == pthread_self())
        event_loop_manage_tasks(event_loop); // 处理动作
    else
        event_loop_wakeup_event(event_loop); // 唤醒可能正在epoll阻塞的子线程

    return 0;
}

int event_loop_add_channel(event_loop_t *event_loop, channel_t *channel)
{
    channel_map_t *channel_map = event_loop->channel_map;

    int ret = channel_map_add(channel_map, channel->fd, channel);
    if (ret == -1)
    {
        perror("event_loop_add_channel");
        return -1;
    }

    event_loop->dispatcher->add(event_loop, channel);
    return 0;
}

int event_loop_remove_channel(event_loop_t *event_loop, channel_t *channel)
{
    channel_map_t *channel_map = event_loop->channel_map;
    if (channel->fd > (channel_map->capacity - 1) || channel_map->list[channel->fd] == NULL)
    {
        perror("event_loop_remove_channel");
        return -1;
    }
    return event_loop->dispatcher->remove(event_loop, channel);
}

int event_loop_destroy_channel(event_loop_t *event_loop, channel_t *channel)
{
    channel_map_t *channel_map = event_loop->channel_map;
    channel_map_remove(channel_map, channel->fd);
    close(channel->fd);
    return 0;
}

int event_loop_modify_channel(event_loop_t *event_loop, channel_t *channel)
{
    channel_map_t *channel_map = event_loop->channel_map;
    if (channel->fd > (channel_map->capacity - 1) || channel_map->list[channel->fd] == NULL)
    {
        perror("event_loop_modify_channel");
        return -1;
    }
    return event_loop->dispatcher->modify(event_loop, channel);
    ;
}

int event_loop_manage_tasks(event_loop_t *event_loop)
{
    pthread_mutex_lock(&(event_loop->mutex));

    while (1)
    {
        channel_task_t *channel_task = (channel_task_t *)linked_list_pop_head(event_loop->task_queue);
        if (channel_task == NULL)
            break;

        if (channel_task->task_type == CHANNEL_TASK_TYPE_ADD)
            event_loop_add_channel(event_loop, channel_task->channel);

        if (channel_task->task_type == CHANNEL_TASK_TYPE_REMOVE)
            event_loop_remove_channel(event_loop, channel_task->channel);

        if (channel_task->task_type == CHANNEL_TASK_TYPE_MDOIFY)
            event_loop_modify_channel(event_loop, channel_task->channel);

        free(channel_task);
    }

    pthread_mutex_unlock(&(event_loop->mutex));

    return 0;
}

int event_loop_process_event(event_loop_t *event_loop, int fd, int type)
{
    if (event_loop == NULL)
    {
        perror("event_loop_process_event");
        return -1;
    }

    if (fd < 0)
    {
        perror("event_loop_process_event fd illegal");
        return -1;
    }

    channel_t *channel = event_loop->channel_map->list[fd];

    // 根据传入的type判断执行读还是写回调
    if (type == CHANNEL_EVENT_READ)
        channel->read_callback(channel->args);

    if (type == CHANNEL_EVENT_WRITE)
        channel->write_callback(channel->args);

    return 0;
}

int event_loop_wakeup_event(event_loop_t *event_loop)
{
    const char *msg = "wakeup signal";
    write(event_loop->socket_pair[0], msg, strlen(msg));
    return 0;
}

int event_loop_socket_pair_read(void *args)
{
    // 这个函数的目的并不是读取数据，而是为了唤醒(解除阻塞)epoll而已，所以读取到了数据之后，什么都不用做

    event_loop_t *event_loop = (event_loop_t *)args;

    char buf[64];
    read(event_loop->socket_pair[1], buf, sizeof(buf));

    return 0;
}
