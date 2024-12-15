#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "server.h"

int main(int argc, char **argv)
{

    if (argc < 3)
    {
        printf("Usage: %s port path\n", argv[0]);
        return -1;
    }

    printf("single reactor server!\n");

    // 切换程序目录到指定的资源目录
    char *rcs_path = argv[2];
    printf("work directory: %s\n", rcs_path);
    chdir(rcs_path);

    // 初始化监听fd
    int listen_fd = init_listen_fd(atoi(argv[1]));
    if (listen_fd == -1)
    {
        perror("init_listen_fd failed");
        return -1;
    }

    // 启动服务器程序
    run_epoll(listen_fd);

    return 0;
}