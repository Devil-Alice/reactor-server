#pragma once
#include "event_loop.h"
#include "dynamic_buffer.h"

/// @brief TCP_connection可以看作channel的父类，处理连接的请求和响应
typedef struct TCP_connection
{
    char name[24];
    event_loop_t *event_loop;
    channel_t *channel;
    dynamic_buffer_t *read_buf;
    dynamic_buffer_t *write_buf;
} TCP_connection_t;

TCP_connection_t *TCP_connection_create(int fd, event_loop_t *event_loop);
void TCP_connection_destroy(TCP_connection_t *TCP_connection);

// 处理处理连接请求的函数
int callback_TCP_connection_read(void *arg_TCP_connection);
// 处理连接响应发送的函数
int callback_TCP_connection_write(void *arg_TCP_connection);
