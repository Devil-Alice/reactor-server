#pragma once
#include "worker_thread.h"

typedef struct thread_pool
{
    event_loop_t *main_event_loop;
    int capacity;
    // 轮询下表：这个线程池是特殊的线程池，分配任务的时候通过下标来轮询地平均分配
    int poll_index;
    bool is_start;
    worker_thread_t *sub_threads;

} thread_pool_t;

thread_pool_t *thread_pool_create(event_loop_t *event_loop, int capacity);
int thread_pool_run(thread_pool_t *thread_pool);
event_loop_t *thread_pool_get_next_event_loop(thread_pool_t *thread_pool);