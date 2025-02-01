#include "TCP_connection.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include "HTTP_request.h"
#include "HTTP_response.h"
#include "log.h"
#include "HTTP_service.h"

// 创建一个TCP_connection对象，创建对应的channel并添加任务
TCP_connection_t *TCP_connection_create(int fd, event_loop_t *event_loop)
{
    TCP_connection_t *TCP_connection = (TCP_connection_t *)malloc(sizeof(TCP_connection_t));
    sprintf(TCP_connection->name, "conn-%d", fd);
    TCP_connection->event_loop = event_loop;
    TCP_connection->read_buf = dynamic_buffer_create(1024);
    TCP_connection->write_buf = dynamic_buffer_create(1024);
    // 创建一个channel，用于处理http请求以及回复http响应，这里将事件设置为读写事件
    channel_t *channel = channel_create(fd, CHANNEL_EVENT_READ | CHANNEL_EVENT_WRITE, callback_TCP_connection_read, callback_TCP_connection_write, callback_TCP_connection_destroy, TCP_connection);
    TCP_connection->channel = channel;
    TCP_connection->response_complete = 0;

    event_loop_add_task(event_loop, channel, CHANNEL_TASK_TYPE_ADD);
    return TCP_connection;
}

void TCP_connection_destroy(TCP_connection_t *TCP_connection)
{
    if (TCP_connection == NULL)
        return;

    if (TCP_connection->read_buf != NULL)
        dynamic_buffer_destroy(TCP_connection->read_buf);

    if (TCP_connection->write_buf != NULL)
        dynamic_buffer_destroy(TCP_connection->write_buf);

    free(TCP_connection);
    return;
}

bool TCP_connection_process_request(TCP_connection_t *TCP_connection)
{

    HTTP_request_t *HTTP_request = HTTP_request_create(TCP_connection);

    int ret = HTTP_service_process(HTTP_request);
    if (ret == -1)
    {
        LOG_DEBUG("TCP_connection_process_request error");
        return false;
    }
    
    // 请求处理完毕,并且已经写入write_buffer
    TCP_connection->response_complete = 1;

    // 此时write_buf中有数据了，但是TCP_connection又不能直接发送
    // 所以需要通过设置channel的读事件为启用，之后添加eventloop的任务
    // 这样dispatch函数检测到channel.fd的epollout事件就会调用channel的write回调函数进行发送响应的操作
    // channel_enable_write_event(TCP_connection->channel, true);
    // event_loop_add_task(TCP_connection->event_loop, TCP_connection->channel, CHANNEL_TASK_TYPE_MDOIFY);

    // HTTP_request在此已经失去了作用，需要释放
    HTTP_request_destroy(HTTP_request);

    // ！注意！：此处的代码逻辑有一个潜在的问题：
    // 当前是通过修改channel的写事件，使得dispatcher在检测到epoll_out的时候会调用我们的写回调函数
    // 但是我们的一个反应堆模型是由一个线程运行的，所以必须等待反应堆处理完读事件，将所有的响应数据写入buffer中，并且修改了channel的事件才能执行写操作
    // 也就是说，我们必须等待所有数据全部写入buffer中才能发送出去
    // 如果数据量不大，那么buffer可以申请空间成功，但是如果发送的数据非常大（例如8GB），那么buffer很有可能申请空间失败
    // 解决方案：需要一边写入buffer一边发送出去，分批次发送，不能等待所有数据准备完毕

    return true;
}

/**
 * @brief 将tcp连接的内容读取出来，，放入tcp_connection的red_buf，并将字符串长度传出
 * @return 返回读取到的字符串长度
 */
int callback_TCP_connection_read(void *arg_TCP_connection)
{
    TCP_connection_t *TCP_connection = (TCP_connection_t *)arg_TCP_connection;
    LOG_DEBUG("%s read connection data", TCP_connection->event_loop->thread_name);

    char buf[1024] = {0};
    int len = 0;
    int total_len = 0;

    //  读写同时发生，需要对临界资源加锁，readbuf只有本函数使用，所以主要是对writebuf上锁
    // 读取函数加锁的地方在构建响应数据函数中：HTTP_response_build
    while (1)
    {
        len = read(TCP_connection->channel->fd, buf, sizeof(buf) - 1);
        if (len == 0)
        {
            // 关闭连接
            return -1;
        }
        else if (len < 0)
        {
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                // 注意：只适用于非阻塞模式，表示当前没有数据可读取，返回
                break;
            }
            else
            {
                perror("callback_TCP_connection_read");
                return -1;
            }
        }

        // 这里传入的buf虽然时栈数据，但是函数内部会将其通过memcpy复制到堆内存中
        dynamic_buffer_append_data(TCP_connection->read_buf, buf, len);
        total_len += len;
    }

    // 处理请求
    TCP_connection_process_request(TCP_connection);

    return total_len;
}

int callback_TCP_connection_write(void *arg_TCP_connection)
{

    TCP_connection_t *TCP_connection = (TCP_connection_t *)arg_TCP_connection;
    LOG_DEBUG("%s write connection data", TCP_connection->event_loop->thread_name);

    // todo: 非常重要！！！！！！！！！！！！！！！！！
    //  有时epoll只检测到了写事件，但是还没检测到读事件，所以这里需要根据情况判断，如果可以单独执行写那么就可以继续执行
    // 如果不能写事件单独执行，例如本程序中的写事件，就会陷入死循环，并且eventloop会等待线程join，就导致本次请求永远无法响应，那么就需要
    if (!(TCP_connection->channel->triggered_events & CHANNEL_EVENT_READ))
    {
        LOG_DEBUG("callback_TCP_connection_write failed: read event is not triggered");
        return -1;
    }

    while (1)
    {
        // todo: 此函数是在一个子线程中执行的，和读函数同时运行，所以要保证该函数在有数据的时候才写入

        // 读取函数加锁的地方在构建响应数据函数中：HTTP_response_build
        pthread_mutex_lock(&TCP_connection->write_buf->mutex);
        // 取出写buf的可读数据
        int max_len = dynamic_buffer_available_read_size(TCP_connection->write_buf);
        if (max_len == 0 && !TCP_connection->response_complete)
        {
            // 没有可读数据，并且响应数据没有完成，此时：释放锁、跳过
            pthread_mutex_unlock(&TCP_connection->write_buf->mutex);
            usleep(1);
            continue;
        }

        char *buf = dynamic_buffer_availabel_read_data(TCP_connection->write_buf);
        // 计算大小，最多一次发送1024字节
        int buf_len = max_len < 1024 ? max_len : 1024;
        int len = write(TCP_connection->channel->fd, buf, buf_len);
        // 读完之后将指针后移
        TCP_connection->write_buf->read_pos += buf_len;
        pthread_mutex_unlock(&TCP_connection->write_buf->mutex);

        // 当len为0时，跳过继续发送，同时如果response_complete那么才退出发送
        if (len == 0)
        {
            if (TCP_connection->response_complete)
                break;
            continue;
        }
        else if (len == -1)
        {
            return -1;
        }
        usleep(1);
    }

    // 处理完成，客户端断开连接的操作应该在epollwait处检测
    event_loop_add_task(TCP_connection->event_loop, TCP_connection->channel, CHANNEL_TASK_TYPE_REMOVE);
    return 0;
}

int callback_TCP_connection_destroy(void *arg_TCP_connection)
{
    TCP_connection_t *TCP_connection = (TCP_connection_t *)arg_TCP_connection;

    TCP_connection_destroy(TCP_connection);
    return 0;
}
