#pragma once
#include <iostream>
#include <string>
#include "channel.hpp"
#include "event_loop.hpp"

class Event_loop;

class Dispatcher
{
protected:
    std::string name = "";
    // Event_loop *event_loop;

public:
    // 内部存放不同的函数指针
    // 初始化
    Dispatcher(std::string name)
    {
        this->name = name;
    }
    // 销毁
    virtual ~Dispatcher() {}

    // 添加
    virtual int add(Channel *channel) = 0;
    // 删除
    virtual int remove(Channel *channel) = 0;
    // 修改
    virtual int modify(Channel *channel) = 0;
    // 事件处理
    virtual int dispatch(Event_loop *event_loop, int timeout_ms = 2000) = 0;
};