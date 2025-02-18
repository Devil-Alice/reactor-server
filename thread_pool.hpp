#pragma once
#include "worker_thread.hpp"
#include <vector>

class Thread_pool
{
private:
    Event_loop *main_event_loop;
    // 线程池的子线程最大数量
    int capacity;
    // 轮询下表：这个线程池是特殊的线程池，分配任务的时候通过下标来轮询地平均分配
    int poll_index;
    bool is_start;
    std::vector<Worker_thread *> sub_threads;

public:
    Thread_pool(Event_loop *main_event_loop, int capacity);
    ~Thread_pool();
    Event_loop *get_main_event_loop() { return main_event_loop; }

    // 线程池运行函数，有主线程执行，但是该函数不会运行主线程的eventloop
    int run();
    // 轮询地取得下一个线程的eventloop，主要是用于平均分配任务
    Event_loop *get_next_event_loop();
};
