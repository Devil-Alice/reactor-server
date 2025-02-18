#pragma once
#include "event_loop.hpp"
#include "dynamic_buffer.hpp"
#include "HTTP_response.hpp"

/// @brief TCP_connection可以看作channel的父类，处理连接的请求和响应
class TCP_connection
{
private:
    std::string name;
    Event_loop *event_loop;
    Channel *channel;
    Dynamic_buffer *read_buf;
    Dynamic_buffer *write_buf;
    /// @brief 表示该响应数据以构建完毕
    int response_complete;

public:
    TCP_connection(int fd, Event_loop *event_loop);
    ~TCP_connection();

    Dynamic_buffer *get_read_buf() { return read_buf; }
    Dynamic_buffer *get_write_buf() { return write_buf; }

    /// @brief 处理请求的函数，根据传入的HTTP_request组织响应数据，最后返回
    /// @param TCP_connection tcp连接用
    /// @param HTTP_response 返回数据：处理好的响应数据，注意时堆内存中的，用完需要释放
    /// @return 是否成功处理
    bool process_request();

    /**
     * @brief 将tcp连接的内容读取出来，，放入tcp_connection的red_buf，并将字符串长度传出
     * @return 返回读取到的字符串长度
     */
    int callback_read();
    // 处理连接响应发送的函数
    int callback_write();
    // 处理连接相关资源的释放
    int callback_destroy();
};
