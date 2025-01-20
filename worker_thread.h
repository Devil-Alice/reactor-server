#pragma once
#include <pthread.h>
#include "event_loop.h"

typedef struct worker_thread
{
    pthread_t id;
    char name[24];
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    event_loop_t *event_loop;

} worker_thread_t;

/**
 * @brief 初始化一个worker_thread对象并分配空间，但是并不创建线程
 * @param num 序号，会用于线程的名称，例如subthread-1
 */
int worker_thread_init(worker_thread_t *worker_thread, int num);
// /**
//  * @brief 创建一个worker_thread对象并分配空间，但是并不创建线程
//  * @param num 序号，会用于线程的名称，例如subthread-1
//  */
// worker_thread_t *worker_thread_create(int num);
/**
 * @brief 被线程执行的函数
 * @param args worker_thread_t的指针
 */
void *threadfunc_event_loop_run(void *args);
int worker_thread_run(worker_thread_t *worker_thread);