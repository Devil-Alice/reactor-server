#include "dispatcher.h"
#include <stdio.h>
#include <unistd.h>
#include <sys/epoll.h>
#include <stdlib.h>

#define EPEVENTS_CAPACITY 512

typedef struct ep_data
{
    int epoll_fd;
    // 这是用来接收epoll_wait传出的事件集合
    struct epoll_event *epoll_events;
} ep_data_t;

static void *epoll_dispatcher_init();
static int epoll_dispatcher_add(event_loop_t *event_loop, channel_t *channel);
static int epoll_dispatcher_remove(event_loop_t *event_loop, channel_t *channel);
static int epoll_dispatcher_modify(event_loop_t *event_loop, channel_t *channel);
static int epoll_dispatcher_dispatch(event_loop_t *event_loop, int timeout_ms);
static int epoll_dispatcher_clear(event_loop_t *event_loop);
static int epoll_dispatcher_ctl(event_loop_t *event_loop, channel_t *channel, int operation);

dispatcher_t epoll_dispatcher = {
    .init = epoll_dispatcher_init,
    .add = epoll_dispatcher_add,
    .remove = epoll_dispatcher_remove,
    .modify = epoll_dispatcher_modify,
    .dispatch = epoll_dispatcher_dispatch,
    .clear = epoll_dispatcher_clear};

static void *epoll_dispatcher_init()
{
    ep_data_t *epdata = malloc(sizeof(struct ep_data));
    epdata->epoll_fd = epoll_create(1);
    if (epdata->epoll_fd == -1)
    {
        perror("epoll_create");
        exit(0);
    }

    epdata->epoll_events = calloc(EPEVENTS_CAPACITY, sizeof(struct epoll_event));

    return epdata;
}

/**
 * @brief 将channel对应的fd，以及events使用epoll_ctl添加到epoll树上
 */
static int epoll_dispatcher_add(event_loop_t *event_loop, channel_t *channel)
{
    return epoll_dispatcher_ctl(event_loop, channel, EPOLL_CTL_ADD);
}

static int epoll_dispatcher_remove(event_loop_t *event_loop, channel_t *channel)
{
    return epoll_dispatcher_ctl(event_loop, channel, EPOLL_CTL_DEL);
}

static int epoll_dispatcher_modify(event_loop_t *event_loop, channel_t *channel)
{
    return epoll_dispatcher_ctl(event_loop, channel, EPOLL_CTL_MOD);
}

static int epoll_dispatcher_dispatch(event_loop_t *event_loop, int timeout_ms)
{
    ep_data_t *epdata = (ep_data_t *)(event_loop->dispatcher_data);
    int cnt = epoll_wait(epdata->epoll_fd, epdata->epoll_events, EPEVENTS_CAPACITY, timeout_ms);
    for (int i = 0; i < cnt; i++)
    {
        int events = epdata->epoll_events[i].events;
        int fd = epdata->epoll_events[i].data.fd;
        // 通过事件判断对方是否断开了链接，EPOLLERR表示文件描述符发生了错误，EPOLLHUP表示挂起，可能是对方关闭了连接，或者是通信出现了问题
        if (events & EPOLLERR || events & EPOLLHUP)
        {
            // todo: 目前还未实现和channel的关联，之后再完成这句代码
            //  epoll_dispatcher_remove(event_loop, channel);
            continue;
        }

        // 触发了读事件
        if (events & EPOLLIN)
            event_loop_process_event(event_loop, fd, FD_EVENT_READ_EVENT);

        // 触发了写事件
        if (events & EPOLLOUT)
            event_loop_process_event(event_loop, fd, FD_EVENT_WRITE_EVENT);
    }
    return 0;
}

static int epoll_dispatcher_clear(event_loop_t *event_loop)
{
    ep_data_t *epdata = (ep_data_t *)(event_loop->dispatcher_data);
    free(epdata->epoll_events);

    close(epdata->epoll_fd);

    free(epdata);
    return 0;
}

/**
 * @brief 控制epoll树的操作，对相应的channel.fd以及epoll树操作
 * @param operation EPOLL_CTL_xxx
 */
int epoll_dispatcher_ctl(event_loop_t *event_loop, channel_t *channel, int operation)
{

    ep_data_t *epdata = (ep_data_t *)(event_loop->dispatcher_data);
    struct epoll_event epevent;
    epevent.data.fd = channel->fd;

    if (channel->events & FD_EVENT_WRITE_EVENT)
        epevent.events |= EPOLLOUT;
    if (channel->events & FD_EVENT_READ_EVENT)
        epevent.events |= EPOLLIN;

    int ret = epoll_ctl(epdata->epoll_fd, operation, channel->fd, &epevent);
    return ret;
}
