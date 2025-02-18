#include "TCP_connection.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include "HTTP_request.hpp"
#include "HTTP_response.hpp"
#include "log.h"
#include "HTTP_service.hpp"

TCP_connection::TCP_connection(int fd, Event_loop *event_loop)
{
    name = "conn-" + std::to_string(fd);
    this->event_loop = event_loop;
    read_buf = new Dynamic_buffer(1024);
    write_buf = new Dynamic_buffer(1024);
    // 创建一个channel，用于处理http请求以及回复http响应，这里将事件设置为读写事件
    auto rcallback = std::bind(&TCP_connection::callback_read, this);
    auto wcallback = std::bind(&TCP_connection::callback_write, this);
    auto dcallback = std::bind(&TCP_connection::callback_destroy, this);
    Channel *channel = new Channel(fd, (CHANNEL_EVENT)((int)CHANNEL_EVENT::READ | (int)CHANNEL_EVENT::WRITE), rcallback, wcallback, dcallback, this);
    this->channel = channel;
    response_complete = 0;

    event_loop->add_task(channel, CHANNEL_TASK_TYPE::ADD);
}

TCP_connection::~TCP_connection()
{

    if (read_buf != NULL)
        delete read_buf;

    if (write_buf != NULL)
        delete write_buf;

    // 注意：此时不能释放channel，虽然是本类创建的channel，但是在eventloop结束channl任务时还需要使用，所以是委托了callback_destroy进行后续的处理
}

bool TCP_connection::process_request()
{

    HTTP_request *request = new HTTP_request(this);

    int ret = HTTP_service::instance().process(request);
    if (ret == -1)
    {
        LOG_DEBUG("TCP_connection_process_request error");
        return false;
    }
    // 请求处理完毕,并且已经写入write_buffer
    response_complete = 1;

    // 此时write_buf中有数据了，但是TCP_connection又不能直接发送
    // 所以需要通过设置channel的读事件为启用，之后添加eventloop的任务
    // 这样dispatch函数检测到channel.fd的epollout事件就会调用channel的write回调函数进行发送响应的操作
    // channel_enable_write_event(channel, true);
    // event_loop_add_task(event_loop, channel, MDOIFY);

    // HTTP_request在此已经失去了作用，需要释放
    delete request;

    // ！注意！：此处的代码逻辑有一个潜在的问题：
    // 当前是通过修改channel的写事件，使得dispatcher在检测到epoll_out的时候会调用我们的写回调函数
    // 但是我们的一个反应堆模型是由一个线程运行的，所以必须等待反应堆处理完读事件，将所有的响应数据写入buffer中，并且修改了channel的事件才能执行写操作
    // 也就是说，我们必须等待所有数据全部写入buffer中才能发送出去
    // 如果数据量不大，那么buffer可以申请空间成功，但是如果发送的数据非常大（例如8GB），那么buffer很有可能申请空间失败
    // 解决方案：需要一边写入buffer一边发送出去，分批次发送，不能等待所有数据准备完毕

    return true;
}

int TCP_connection::callback_read()
{
    LOG_DEBUG("TCP_connection(%s) read data", event_loop->get_thread_name().data());

    //  同时创建写数据的线程
    pthread_t write_thread_id = -1;
    // event_loop_process_event(event_loop, fd, WRITE);
    Arg_event_data *event_data_write = new Arg_event_data();
    event_data_write->event_loop = event_loop;
    event_data_write->fd = channel->get_fd();
    event_data_write->type = CHANNEL_EVENT::WRITE;
    pthread_create(&write_thread_id, NULL, threadfunc_event_loop_process_event, event_data_write);

    // 开始读取
    char buf[1024] = {0};
    int len = 0;
    int total_len = 0;

    //  读写同时发生，需要对临界资源加锁，readbuf只有本函数使用，所以主要是对writebuf上锁
    // 读取函数加锁的地方在构建响应数据函数中：HTTP_response_build
    while (1)
    {
        len = read(channel->get_fd(), buf, sizeof(buf) - 1);
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
                LOG_ERROR("callback_TCP_connection_read");
                return -1;
            }
        }

        // 这里传入的buf虽然时栈数据，但是函数内部会将其通过memcpy复制到堆内存中
        read_buf->append_string(buf, len);
        total_len += len;
    }

    // 处理请求
    process_request();

    // 最后等待写数据的线程执行完毕
    pthread_join(write_thread_id, NULL);

    return total_len;
}

int TCP_connection::callback_write()
{
    // TCP_connection *tcp_connection = (TCP_connection *)arg_TCP_connection;
    LOG_DEBUG("TCP_connection(%s) write data", event_loop->get_thread_name().data());

    // todo: 非常重要！！！！！！！！！！！！！！！！！
    //  有时epoll只检测到了写事件，但是还没检测到读事件，所以这里需要根据情况判断，如果可以单独执行写那么就可以继续执行
    // 如果不能写事件单独执行，例如本程序中的写事件，就会陷入死循环，并且eventloop会等待线程join，就导致本次请求永远无法响应，那么就需要
    // if (!(channel->triggered_events & READ))
    // {
    //     LOG_DEBUG("callback_TCP_connection_write failed: read event is not triggered");
    //     return -1;
    // }

    int total_len = 0;

    while (1)
    {
        // todo: 此函数是在一个子线程中执行的，和读函数同时运行，所以要保证该函数在有数据的时候才写入

        // 读取函数加锁的地方在构建响应数据函数中：HTTP_response_build
        write_buf->get_mutex()->lock();
        // 取出写buf的可读数据
        int max_len = write_buf->available_read_size();
        if (max_len == 0 && !response_complete)
        {
            // 没有可读数据，并且响应数据没有完成，此时：释放锁、跳过
            write_buf->get_mutex()->unlock();
            usleep(1);
            continue;
        }

        char *buf = write_buf->availabel_read_data();
        // 计算大小，最多一次发送1024字节
        int buf_len = max_len < 10240 ? max_len : 10240;
        int len = send(channel->get_fd(), buf, buf_len, MSG_NOSIGNAL);
        // 读完之后将指针后移
        if (len > 0)
        {
            write_buf->set_read_pos(write_buf->get_read_pos() + len);
            total_len += len;
        }
        write_buf->get_mutex()->unlock();

        // 当len为0时，跳过继续发送，同时如果response_complete那么才退出发送
        if (len == 0)
        {
            if (response_complete)
                break;
            continue;
        }
        else if (len == -1)
        {
            // 这两个错误是因为缓冲区已满导致的，所以不需要结束通信，而是等待一会继续发送
            if (errno == EAGAIN || errno == EWOULDBLOCK)
            {
                usleep(3);
                continue;
            }
            perror("err");
            // send时设置msg_nosignal只是让内核不要在啊触发错误信号时杀死本程序进程，所以仍要处理错误码
            //  LOG_ERROR("tcp connection write");
            if (errno == EPIPE || errno == ECONNRESET)
                break;
            return -1;
        }
    }

    // 处理完成，客户端断开连接的操作应该在epollwait处检测
    event_loop->add_task(channel, CHANNEL_TASK_TYPE::REMOVE);
    return 0;
}

int TCP_connection::callback_destroy()
{
    // TCP_connection *tcp_connection = (TCP_connection *)arg_TCP_connection;

    return 0;
}
