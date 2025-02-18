#include "thread_pool.hpp"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "log.h"

Thread_pool::Thread_pool(Event_loop *main_event_loop, int capacity)
{
    this->main_event_loop = main_event_loop;
    is_start = false;
    this->capacity = capacity;
    poll_index = 0;
    sub_threads.clear();
}

Thread_pool::~Thread_pool()
{
}

int Thread_pool::run()
{
    // 检查线程池
    assert(!is_start);

    // 线程池必须由主线程启动，所以需要检查当前运行的是不是主线程
    if (main_event_loop->get_thread_id() != std::this_thread::get_id())
    {
        LOG_ERROR("thread_pool_run");
        return -1;
    }

    is_start = true;

    for (int i = 0; i < capacity; i++)
    {
        Worker_thread *worker_thread = new Worker_thread("subthread-" + std::to_string(i));
        sub_threads.push_back(worker_thread);
        worker_thread->run();
    }

    return 0;
}

Event_loop *Thread_pool::get_next_event_loop()
{
    // 检查线程池
    assert(is_start);

    // 必须由主线程执行，所以需要检查当前运行的是不是主线程
    // if (thread_pool->main_event_loop->thread_id != pthread_self())
    // {
    //     LOG_ERROR("thread_pool_run");
    //     return NULL;
    // }

    // 如果线程池没有子线程时，应该由主线程的event_loop来处理事件，也就是单反应堆模式
    Event_loop *event_loop = main_event_loop;
    if (capacity > 0)
    {
        event_loop = sub_threads[poll_index]->get_event_loop();
        poll_index = (++poll_index) % capacity;
    }

    return event_loop;
}
