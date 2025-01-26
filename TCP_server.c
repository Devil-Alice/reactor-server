#include "TCP_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include "TCP_connection.h"
#include "log.h"

TCP_server_t *TCP_server_create(unsigned short port, int threads_capacity)
{
    LOG_DEBUG("create TCP server");
    TCP_server_t *TCP_server = (TCP_server_t *)malloc(sizeof(TCP_server_t));
    TCP_server->thread_pool = thread_pool_create(event_loop_create_main(), 10);
    TCP_server->listener = listener_create(port);
    LOG_DEBUG("create TCP server successfully");
    return TCP_server;
}

listener_t *listener_create(unsigned short port)
{
    LOG_DEBUG("create listener");
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {

        perror("TCP_server_create socket");
        return NULL;
    }

    // 设置端口复用，SOL(socket option level)为socket，SO为reuse addr，optval相当于enable开启
    int optval = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    if (ret == -1)
    {
        perror("TCP_server_create setsockopt");
        return NULL;
    }

    struct sockaddr_in sockaddr = {
        .sin_addr.s_addr = INADDR_ANY,
        .sin_family = AF_INET,
        .sin_port = htons(port),
    };
    ret = bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr_in));
    if (ret == -1)
    {
        perror("TCP_server_create bind");
        return NULL;
    }
    ret = listen(sockfd, 128);
    if (ret == -1)
    {
        perror("TCP_server_create listen");
        return NULL;
    }

    // 构建listener
    listener_t *listener = (listener_t *)malloc(sizeof(listener_t));
    listener->port = port;
    listener->fd = sockfd;

    LOG_DEBUG("create listener successfully");
    return listener;
}

int callback_TCP_server_accept(void *arg_TCP_server)
{

    TCP_server_t *TCP_server = (TCP_server_t *)arg_TCP_server;

    int listen_fd = TCP_server->listener->fd;
    int client_fd = accept(listen_fd, NULL, NULL);
    if (client_fd == -1)
    {
        perror("callback_TCP_server_accept");
        return -1;
    }

    event_loop_t *next_event_loop = thread_pool_get_next_event_loop(TCP_server->thread_pool);
    TCP_connection_t *TCP_connection = TCP_connection_create(client_fd, next_event_loop);

    return 0;
}

int TCP_server_run(TCP_server_t *TCP_server)
{
    LOG_DEBUG("TCP server start");

    // 构建channel，添加任务，让eventloop检测读事件，因为是读取客户端的连接，所以只需要传入读回调，服务器关闭时会自动销毁
    channel_t *channel = channel_create(TCP_server->listener->fd, CHANNEL_EVENT_READ, callback_TCP_server_accept, NULL, NULL, TCP_server);

    event_loop_add_task(TCP_server->thread_pool->main_event_loop, channel, CHANNEL_TASK_TYPE_ADD);

    // 启动所有子线程的eventloop
    thread_pool_run(TCP_server->thread_pool);

    // 单独启用主线程的eventloop
    LOG_DEBUG("main event loop is running");
    LOG_DEBUG("TCP server is running");
    event_loop_run(TCP_server->thread_pool->main_event_loop);
    
    return 0;
}
