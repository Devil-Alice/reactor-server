#include "channel.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>

channel_t *channel_create(int fd, int events, channel_handle_func rcallback, channel_handle_func wcallback, channel_handle_func dcallback, void *args)
{
    channel_t *channel = (channel_t *)malloc(sizeof(channel_t));
    channel->fd = fd;
    channel->events = events;
    channel->triggered_events = 0;
    channel->read_callback = rcallback;
    channel->write_callback = wcallback;
    channel->destroy_callback = dcallback;
    channel->args = args;
    return channel;
}

void channel_destroy(channel_t *channel)
{
    if (channel == NULL)
        return;

    // 这里的args交给销毁回调释放
    //  if (channel->args != NULL)
    //      free(channel->args);

    free(channel);
    channel == NULL;

    return;
}

void channel_enable_write_event(channel_t *channel, bool flag)
{
    if (flag == true)
        channel->events |= CHANNEL_EVENT_WRITE;
    else
        channel->events &= ~CHANNEL_EVENT_WRITE;
    return;
}

bool channel_get_write_event_status(channel_t *channel)
{
    return channel->events & CHANNEL_EVENT_WRITE;
}
