#pragma once
#include "event_loop.h"
#include "dynamic_buffer.h"

typedef struct TCP_connection
{
    char name[24];
    event_loop_t *event_loop;
    channel_t *channel;
    dynamic_buffer_t *read_buf;
    dynamic_buffer_t *write_buf;
} TCP_connection_t;

TCP_connection_t *TCP_connection_create(int fd, event_loop_t *event_loop);
int callback_TCP_connection_read(void *arg_TCP_connection);