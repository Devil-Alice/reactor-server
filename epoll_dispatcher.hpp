#pragma once
#include "dispatcher.hpp"

#define EPEVENTS_CAPACITY 512

class Epoll_data
{
public:
    int epoll_fd;
    // 这是用来接收epoll_wait传出的事件集合，通过calloc申请，需要在析构中释放
    struct epoll_event *epoll_events;

    Epoll_data();
    ~Epoll_data();
};

class Epoll_dispatcher : public Dispatcher
{
public:
    Epoll_dispatcher(std::string name);
    ~Epoll_dispatcher();
    /**
     * @brief 将channel对应的fd，以及events使用epoll_ctl添加到epoll树上
     */
    int add(Channel *channel) override;
    // 删除
    int remove(Channel *channel) override;
    // 修改
    int modify(Channel *channel) override;
    // 事件处理
    int dispatch(Event_loop *event_loop, int timeout_ms = 2000) override;

private:
    Epoll_data epoll_data;
    /**
     * @brief 控制epoll树的操作，对相应的channel.fd以及epoll树操作
     * @param operation EPOLL_CTL_xxx
     */
    int control(Channel *channel, int operation);
};