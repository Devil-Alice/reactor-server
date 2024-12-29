#pragma once
#include "channel.h"
#include "event_loop.h"

typedef struct dispatcher
{
    // 内部存放不同的函数指针
    // 初始化
    void *(*init)();
    // 添加
    int (*add)(event_loop_t *event_loop, channel_t *channel);
    // 删除
    int (*remove)(event_loop_t *event_loop, channel_t *channel);
    // 修改
    int (*modify)(event_loop_t *event_loop, channel_t *channel);
    // 事件处理
    int (*dispatch)(event_loop_t *event_loop, int timeout_ms);
    // 清除
    int (*clear)(event_loop_t *event_loop);
} dispatcher_t;