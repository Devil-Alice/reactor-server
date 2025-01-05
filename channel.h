#pragma once
#include <stdbool.h>

enum FD_EVENT
{
    FD_EVENT_TIMEOUT = 0x01,
    FD_EVENT_READ_EVENT = 0x02,
    FD_EVENT_WRITE_EVENT = 0x04
};

typedef void *(*channel_handle_func)(int arg);

typedef struct channel
{
    int fd;
    int events;
    void *args;
    channel_handle_func read_callback;
    channel_handle_func write_callback;
} channel_t;

channel_t *channel_init(int fd, int events, channel_handle_func rcallback, channel_handle_func wcallback);

int enable_channel_write_event(channel_t *channel, bool flag);

bool is_enable_channel_write_event(channel_t *channel);