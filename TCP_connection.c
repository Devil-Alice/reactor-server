#include "TCP_connection.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

// 创建一个TCP_connection对象，创建对应的channel并添加任务
TCP_connection_t *TCP_connection_create(int fd, event_loop_t *event_loop)
{
    TCP_connection_t *TCP_connection = (TCP_connection_t *)malloc(sizeof(TCP_connection_t));
    sprintf(TCP_connection->name, "conn-%d", fd);
    TCP_connection->event_loop = event_loop;
    TCP_connection->read_buf = dynamic_buffer_create(1024);
    TCP_connection->read_buf = dynamic_buffer_create(1024);

    channel_t *channel = channel_create(fd, CHANNEL_EVENT_READ, callback_TCP_connection_read, NULL, TCP_connection);
    TCP_connection->channel = channel;
    event_loop_add_task(event_loop, channel, CHANNEL_TASK_TYPE_ADD);

    return NULL;
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

    // 接收到了数据后，就需要对接收到的数据进行处理
    // 处理函数,解析http请求的内容

    return total_len;
}
