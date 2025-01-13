#pragma once
#include <stdbool.h>

enum CHANNEL_EVENT
{
    CHANNEL_EVENT_TIMEOUT = 0x01,
    CHANNEL_EVENT_READ = 0x02,
    CHANNEL_EVENT_WRITE = 0x04
};

typedef int (*channel_handle_func)(void *args);

typedef struct channel
{
    int fd;
    int events;
    void *args;
    channel_handle_func read_callback;
    channel_handle_func write_callback;
} channel_t;

channel_t *channel_create(int fd, int events, channel_handle_func rcallback, channel_handle_func wcallback, void *args);

int enable_channel_write_event(channel_t *channel, bool flag);

bool is_enable_channel_write_event(channel_t *channel);