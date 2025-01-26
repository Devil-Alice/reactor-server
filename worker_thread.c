#include "worker_thread.h"
#include <stdio.h>

int worker_thread_init(worker_thread_t *worker_thread, int num)
{
    worker_thread->id = 0;
    sprintf(worker_thread->name, "subthread-%d", num);
    pthread_mutex_init(&worker_thread->mutex, NULL);
    pthread_cond_init(&worker_thread->cond, NULL);
    worker_thread->event_loop = NULL;
    return 0;
}

// worker_thread_t *worker_thread_create(int num)
// {
//     worker_thread_t *worker_thread = malloc(sizeof(worker_thread_t));
//     worker_thread->id = 0;
//     sprintf(worker_thread->name, "subthread-&d", num);
//     pthread_mutex_init(&worker_thread->mutex, NULL);
//     pthread_cond_init(&worker_thread->cond, NULL);
//     worker_thread->event_loop = NULL;

//     return worker_thread;
// }

void *threadfunc_event_loop_run(void *args)
{
    worker_thread_t *worker_thread = (worker_thread_t *)args;
    pthread_mutex_lock(&worker_thread->mutex);
    worker_thread->event_loop = event_loop_create(worker_thread->name);
    pthread_mutex_unlock(&worker_thread->mutex);
    pthread_cond_signal(&worker_thread->cond);
    if (worker_thread->event_loop == NULL)
    {
        perror("threadfunc_event_loop_run");
        return (void *)-1;
    }

    event_loop_run(worker_thread->event_loop);

    return (void *)0;
}

int worker_thread_run(worker_thread_t *worker_thread)
{
    // 注意：这里创建好子线程后，并不一定会立刻执行
    // worker_thread_run这个函数是主线程执行的，而eventloop时在子线程中初始化的
    // 也就意味着线程函数中对于eventloop的初始化还未完成，此时主线程如果访问eventloop就会报错
    // 所以，需要使用条件变量cond来确保eventloop正确初始化完成
    pthread_create(&worker_thread->id, NULL, threadfunc_event_loop_run, worker_thread);

    // 在这里等待eventloop初始化完毕
    pthread_mutex_lock(&worker_thread->mutex);
    while (worker_thread->event_loop == NULL)
    {
        pthread_cond_wait(&worker_thread->cond, &worker_thread->mutex);
    }
    pthread_mutex_unlock(&worker_thread->mutex);

    //此线程其他操作这时候就可以正常访问eventloop了
    //例如：主线程（也就是主反应堆）在接收到了一个链接请求后，会访问分配给子线程的反应堆（eventloop）
    return 0;
}
