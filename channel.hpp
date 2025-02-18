#pragma once
#include <iostream>
#include <functional>

enum class CHANNEL_EVENT
{
    TIMEOUT = 0x01,
    READ = 0x02,
    WRITE = 0x04,
    DESTROY = 0x08
};
CHANNEL_EVENT operator|(CHANNEL_EVENT l_hand_side, CHANNEL_EVENT r_hand_side);
CHANNEL_EVENT operator&(CHANNEL_EVENT l_hand_side, CHANNEL_EVENT r_hand_side);


class Channel
{
private:
    int fd;
    CHANNEL_EVENT events;
    void *args;

public:
    // typedef int (*channel_handle_func)(void *);//不支持模版
    // using支持模版，typedef不行
    // 要在cpp的成员函数中传入函数指针会遇到一个问题：
    // cpp中的一般的成员函数是属于类对象的，它定义了，但是当类对象没有创建时，它并不存在，所以无法作为函数指针传入参数，这时候就需要使用function
    // function是c++11的新特性：可调用对象包装器，将函数指针的定义使用function包装，需要指定她的范型参数为一个可调用对象，或者是一个函数指针,
    // 在传入通过function定义的函数指针时，需要使用bind
    using channel_handle_func = std::function<int(void *)>;
    channel_handle_func read_callback;
    channel_handle_func write_callback;
    channel_handle_func destroy_callback;

    inline int get_fd() { return fd; } // 【类内】定义的函数可以不用谢inline，因为默认就是inline
    CHANNEL_EVENT get_events() { return events; };
    const void *get_args() { return args; }; // 加上const不允许修改args

    Channel(int fd, CHANNEL_EVENT events, channel_handle_func rcallback, channel_handle_func wcallback, channel_handle_func dcallback, void *args);
    ~Channel();

    /// @brief 设置channel的写事件状态
    /// @param channel
    /// @param flag true为开启，false为关闭
    /// @return
    void enable_write_event(bool flag);
    /// @brief 获取channel的写事件转改
    /// @param channel
    /// @return true为开启，false为关闭
    bool get_write_event_status();
};
