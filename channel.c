#include "channel.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/epoll.h>

channel_t *channel_create(int fd, int events, channel_handle_func rcallback, channel_handle_func wcallback, void *args)
{
    channel_t *channel = (channel_t *)malloc(sizeof(channel_t));
    channel->fd = fd;
    channel->events = events;
    channel->read_callback = rcallback;
    channel->write_callback = wcallback;
    channel->args = args;
    return NULL;
}

int enable_channel_write_event(channel_t *channel, bool flag)
{
    if (flag == true)
        channel->events |= CHANNEL_EVENT_WRITE;
    else 
        channel->events &= ~CHANNEL_EVENT_WRITE;
    return 0;
}

bool is_enable_channel_write_event(channel_t *channel)
{
    return channel->events & CHANNEL_EVENT_WRITE;
}
