#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "TCP_server.hpp"
#include "log.h"

// gcc *.c -o ./build/main -lpthread -std=gnu11
int main(int argc, char **argv)
{

    if (argc < 3)
    {
        printf("Usage: %s port path\n", argv[0]);
        return -1;
    }

    printf("multi reactor server!\n");
    // 解析传入参数
    int port = atoi(argv[1]);
    char *rcs_path = argv[2];

    // 切换程序目录到指定的资源目录
    printf("work directory: %s\n", rcs_path);
    chdir(rcs_path);

    // 启动多反应堆模型
    TCP_server *tcp_server = new TCP_server(port, 2);
    tcp_server->run(); 

    return 0;
}