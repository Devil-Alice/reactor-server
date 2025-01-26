#include "thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "log.h"

thread_pool_t *thread_pool_create(event_loop_t *main_event_loop, int capacity)
{

    thread_pool_t *thread_pool = (thread_pool_t *)malloc(sizeof(thread_pool_t));
    thread_pool->main_event_loop = main_event_loop;
    thread_pool->is_start = false;
    thread_pool->capacity = capacity;
    thread_pool->poll_index = 0;
    thread_pool->sub_threads = (worker_thread_t *)malloc(sizeof(worker_thread_t) * capacity);
    return thread_pool;
}

int thread_pool_run(thread_pool_t *thread_pool)
{
    LOG_DEBUG("thread pool start");
    // 检查线程池
    assert(thread_pool && !(thread_pool->is_start));

    // 线程池必须由主线程启动，所以需要检查当前运行的是不是主线程
    if (thread_pool->main_event_loop->thread_id != pthread_self())
    {
        perror("thread_pool_run");
        return -1;
    }

    thread_pool->is_start = true;

    for (int i = 0; i < thread_pool->capacity; i++)
    {
        worker_thread_init(&(thread_pool->sub_threads[i]), i);
        worker_thread_run(&(thread_pool->sub_threads[i]));
    }

    LOG_DEBUG("thread pool is running");
    return 0;
}

event_loop_t *thread_pool_get_next_event_loop(thread_pool_t *thread_pool)
{
    // 检查线程池
    assert(thread_pool && thread_pool->is_start);

    // 必须由主线程执行，所以需要检查当前运行的是不是主线程
    if (thread_pool->main_event_loop->thread_id != pthread_self())
    {
        perror("thread_pool_run");
        return NULL;
    }

    // 如果线程池没有子线程时，应该由主线程的event_loop来处理事件，也就是单反应堆模式
    event_loop_t *event_loop = thread_pool->main_event_loop;
    if (thread_pool->capacity > 0)
    {
        event_loop = thread_pool->sub_threads[thread_pool->poll_index].event_loop;
        thread_pool->poll_index = (++thread_pool->poll_index) % thread_pool->capacity;
    }

    return event_loop;
}
