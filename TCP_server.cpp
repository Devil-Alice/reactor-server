#include "TCP_server.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <sys/socket.h>
#include "TCP_connection.hpp"
#include "HTTP_service.hpp"
#include "log.h"

TCP_server::TCP_server(unsigned short port, int threads_capacity)
{
    this->port = port;
    int ret = start_listen();
    if (ret == -1)
        LOG_ERROR("listen failed");
    
    thread_pool = new Thread_pool(new Event_loop(""), threads_capacity);
}

int TCP_server::accept_client()
{
    int client_fd = accept(listen_fd, NULL, NULL);

    if (client_fd == -1)
    {
        LOG_ERROR("callback_TCP_server_accept");
        return -1;
    }

    LOG_DEBUG("TCP_server listen fd(%d) accpet a new client fd(%d)", listen_fd, client_fd);

    // 设置非阻塞模式
    int flag = fcntl(client_fd, F_GETFL);
    flag |= O_NONBLOCK;
    fcntl(client_fd, F_SETFL, flag);

    // 设置超时时间10s
    struct timeval timeout = {.tv_sec = 10, .tv_usec = 0};
    setsockopt(client_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    Event_loop *next_event_loop = thread_pool->get_next_event_loop();
    TCP_connection *tcp_connection = new TCP_connection(client_fd, next_event_loop);

    return 0;
}

int TCP_server::start_listen()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1)
    {
        LOG_ERROR("TCP_server_create socket");
        return -1;
    }

    // 设置端口复用，SOL(socket option level)为socket，SO为reuse addr，optval相当于enable开启
    int optval = 1;
    int ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval));
    // ret = setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof(optval));
    if (ret == -1)
    {
        LOG_ERROR("TCP_server_create setsockopt");
        return -1;
    }

    sockaddr_in sockaddr;
    sockaddr.sin_addr.s_addr = INADDR_ANY;
    sockaddr.sin_family = AF_INET;
    sockaddr.sin_port = htons(port);

    ret = bind(sockfd, (struct sockaddr *)&sockaddr, sizeof(struct sockaddr_in));
    if (ret == -1)
    {
        LOG_ERROR("TCP_server_create bind");
        return -1;
    }

    ret = listen(sockfd, 128);
    if (ret == -1)
    {
        LOG_ERROR("TCP_server_create listen");
        return -1;
    }

    // 初始化
    this->listen_fd = sockfd;

    return 0;
}

int TCP_server::run()
{

    // 构建channel，添加任务，让eventloop检测读事件，因为是读取客户端的连接，所以只需要传入读回调，服务器关闭时会自动销毁
    auto func_ptr = std::bind(&TCP_server::accept_client, this);
    Channel *channel = new Channel(listen_fd, CHANNEL_EVENT::READ, func_ptr, nullptr, nullptr, this);

    Event_loop *main_eventloop = thread_pool->get_main_event_loop();
    main_eventloop->add_task(channel, CHANNEL_TASK_TYPE::ADD);

    // 启动所有子线程的eventloop
    thread_pool->run();

    // 单独启用主线程的eventloop
    thread_pool->get_main_event_loop()->run();

    return 0;
}
