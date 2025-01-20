#include "TCP_connection.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "HTTP_request.h"
#include "HTTP_response.h"

// 创建一个TCP_connection对象，创建对应的channel并添加任务
TCP_connection_t *TCP_connection_create(int fd, event_loop_t *event_loop)
{
    TCP_connection_t *TCP_connection = (TCP_connection_t *)malloc(sizeof(TCP_connection_t));
    sprintf(TCP_connection->name, "conn-%d", fd);
    TCP_connection->event_loop = event_loop;
    TCP_connection->read_buf = dynamic_buffer_create(1024);
    TCP_connection->read_buf = dynamic_buffer_create(1024);
    // 创建一个channel，用于处理http请求以及回复http响应，这里将事件设置为读写事件
    channel_t *channel = channel_create(fd, CHANNEL_EVENT_READ & CHANNEL_EVENT_WRITE, callback_TCP_connection_read, callback_TCP_connection_write, TCP_connection);
    TCP_connection->channel = channel;

    event_loop_add_task(event_loop, channel, CHANNEL_TASK_TYPE_ADD);
    return TCP_connection;
}

void TCP_connection_destroy(TCP_connection_t *TCP_connection)
{
    if (TCP_connection == NULL)
        return;

    channel_destroy(TCP_connection->channel);

    if (TCP_connection->read_buf != NULL)
        dynamic_buffer_destroy(TCP_connection->read_buf);

    if (TCP_connection->write_buf != NULL)
        dynamic_buffer_destroy(TCP_connection->write_buf);

    free(TCP_connection);
    return;
}

/**
 * @brief 将tcp连接的内容读取出来，，放入tcp_connection的red_buf，并将字符串长度传出
 * @return 返回读取到的字符串长度
 */
int callback_TCP_connection_read(void *arg_TCP_connection)
{
    TCP_connection_t *TCP_connection = (TCP_connection_t *)arg_TCP_connection;

    char buf[1024] = {0};
    int len = 0;
    int total_len = 0;

    while (1)
    {
        len = read(TCP_connection->channel->fd, buf, sizeof(buf) - 1);
        if (len == 0)
        {
            // 断开连接
            return 0;
        }
        else if (len < 0)
        {
            perror("callback_TCP_connection_read");
            return -1;
        }

        // 这里传入的buf虽然时栈数据，但是函数内部会将其通过memcpy复制到堆内存中
        dynamic_buffer_append(TCP_connection->read_buf, buf);
        total_len += len;
    }

    // 接收到了数据后，就需要对接收到的请求进行解析
    // 解析函数,解析http请求的内容
    HTTP_request_t *HTTP_request = HTTP_request_create();
    HTTP_request_parse_reqest(HTTP_request, TCP_connection->read_buf);
    // 到目前为止HTTP_request已经有了请求的各种数据，例如method,url等，接下来就是处理这个请求，该如何给客户端发送响应
    // 处理http请求的函数：
    // todo: 这里的process函数最好另外创建一个文件单独写，并且，处理各种url的函数也写在内部，通过一个链表存放url与其处理函数的关系
    HTTP_response_t *HTTP_response = HTTP_response_create();
    HTTP_request_process_request(HTTP_request, HTTP_response);
    // 将获取到的响应构建成字符串写入write_buf
    HTTP_response_build(HTTP_response, TCP_connection->write_buf);

    // 此时write_buf中有数据了，但是TCP_connection又不能直接发送
    // 所以需要通过设置channel的读事件为启用，之后添加eventloop的任务
    // 这样dispatch函数检测到channel.fd的epollout事件就会调用channel的write回调函数进行发送响应的操作
    channel_enable_write_event(TCP_connection->channel, true);
    event_loop_add_task(TCP_connection->event_loop, TCP_connection->channel, CHANNEL_TASK_TYPE_MDOIFY);

    // HTTP_response和HTTP_request在此已经失去了作用，需要释放
    HTTP_response_destroy(HTTP_response);
    HTTP_request_destroy(HTTP_request);

    // ！注意！：此处的代码逻辑有一个潜在的问题：
    // 当前是通过修改channel的写事件，使得dispatcher在检测到epoll_out的时候会调用我们的写回调函数
    // 但是我们的一个反应堆模型是由一个线程运行的，所以必须等待反应堆处理完读事件，将所有的响应数据写入buffer中，并且修改了channel的事件才能执行写操作
    // 也就是说，我们必须等待所有数据全部写入buffer中才能发送出去
    // 如果数据量不大，那么buffer可以申请空间成功，但是如果发送的数据非常大（例如8GB），那么buffer很有可能申请空间失败
    // 解决方案：需要一边写入buffer一边发送出去，分批次发送，不能等待所有数据准备完毕
    // todo: 完成边写边发

    return total_len;
}

int callback_TCP_connection_write(void *arg_TCP_connection)
{
    TCP_connection_t *TCP_connection = (TCP_connection_t *)arg_TCP_connection;

    while (1)
    {
        // 取出写buf的可读数据
        char *buf = dynamic_buffer_availabel_read_data(TCP_connection->write_buf);
        int max_len = dynamic_buffer_available_read_size(TCP_connection->write_buf);
        // 计算大小，最多一次发送1024字节
        int buf_len = max_len < 1024 ? max_len : 1024;
        int len = write(TCP_connection->channel->fd, buf, buf_len);
        // 读完之后将指针后移
        TCP_connection->write_buf->read_pos += buf_len;

        if (len == 0)
        {
            break;
        }
        else if (len == -1)
        {
            break;
        }
    }

    // 处理完成，客户端断开连接的操作应该在epollwait处检测
    // event_loop_add_task(TCP_connection->event_loop, TCP_connection->channel, CHANNEL_TASK_TYPE_REMOVE);
    return 0;
}
