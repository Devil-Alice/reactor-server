#pragma once
#include "linked_list.h"
#include <pthread.h>

typedef struct fd_info
{
    pthread_t tid;
    int epoll_fd;
    int listen_fd;
    int client_fd;

} fd_info;

/**
 * @brief 初始化一个监听socket_fd并将其返回
 * @param port 指定监听的端口
 * @return 返回初始化的fd；-1表示失败
 */
int init_listen_fd(unsigned short port);

/**
 * @brief 运行epoll监听fd
 */
int run_epoll(int listen_fd);

/**
 * @brief 接收客户端的连接，并将其放入epoll监听
 */
int accept_client(int epoll_fd, int listen_fd);

/**
 * @param fdInfo FdInfo类型数据
 * @note 使用完fdInfo后，需要释放
 */
void *thread_accept_client(void *arg_fd_info);

int recv_request(int epoll_fd, int client_fd);

void *thread_recv_request(void *arg_fd_info);

/**
 * @brief 发送响应
 * @param client_fd 客户端的fd
 * @param status 响应的状态码，例如200、404、500等
 * @param description 状态码的描述，例如OK、Not Found、Internal Server Error等
 * @param type 响应的类型，例如text/plain、application/json等
 * @param len 响应的数据长度
 * @note
 * 最简单的响应内容应该分为以下几个部分：
 *
 * 响应行：HTTP/1.1 状态吗 状态码描述
 *
 * 响应头：Content-type: 数据类型
 *
 * 响应数据长度：Content-Length: 响应的数据长度
 *
 * 空行：\r\n
 *
 * 响应体：响应的数据内容
 *
 */
int send_response_header(int client_fd, int status, const char *description, const char *type, int len);

int handle_request(int client_fd, linked_list *list);

int send_file(int client_fd, char *file_name);

int send_dir(int client_fd, char *dir_name);