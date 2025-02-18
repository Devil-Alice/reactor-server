#pragma once
#include <sys/socket.h>
#include <sys/types.h>
#include "thread_pool.hpp"


class TCP_server
{
private:
    int listen_fd;
    unsigned short port;
    Thread_pool *thread_pool;

public:
    TCP_server(unsigned short port, int threads_capacity);
    int accept_client();

    int start_listen();
    // 启动服务器的函数，先单独启动主线程，目的是为了listen_fd单独创建一个eventloop并执行监听
    // 之后再启动所有的字线程，子线程用于正常通信
    int run();
};
