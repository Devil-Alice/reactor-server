#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include "thread_pool.h"

typedef struct listener
{
    int fd;
    unsigned short port;
} listener_t;

typedef struct TCP_server
{
    thread_pool_t *thread_pool;
    listener_t *listener;
} TCP_server_t;

TCP_server_t *TCP_server_create(unsigned short port, int threads_capacity);
listener_t *listener_create(unsigned short port);
int callback_TCP_server_accept(void *arg_TCP_server);
// 启动服务器的函数，先单独启动主线程，目的是为了listen_fd单独创建一个eventloop并执行监听
// 之后再启动所有的字线程，子线程用于正常通信
int TCP_server_run(TCP_server_t *TCP_server);