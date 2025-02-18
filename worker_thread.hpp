#pragma once
#include <iostream>
#include <thread>
#include <condition_variable>
#include "event_loop.hpp"

class Worker_thread
{
private:
    std::thread *thread;
    std::thread::id id;
    std::string name;
    std::mutex mutex;
    std::condition_variable cond;
    Event_loop *event_loop;

public:
    Worker_thread(std::string name);
    ~Worker_thread();
    Event_loop *get_event_loop() { return event_loop; }

    /**
     * @brief 被线程执行的函数
     * @param args worker_thread_t的指针
     */
    void run();
    void task_func(void *args);
};
