#include "event_loop.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <assert.h>

event_loop_t *event_loop_create_main(void)
{
    return event_loop_create(NULL);
}

event_loop_t *event_loop_create(char *thread_name)
{

    event_loop_t *event_loop = (event_loop_t *)malloc(sizeof(struct event_loop));
    event_loop->is_quit = false;
    // 将epoll的dispatcher赋值给eventloop
    event_loop->dispatcher = &epoll_dispatcher;
    // 使用dispatcher的init函数初始化data并获取
    event_loop->dispatcher_data = event_loop->dispatcher->init();

    // 初始化任务队列
    event_loop->task_queue = linked_list_create();

    // 初始化channel_map
    event_loop->channel_map = channel_map_create(128);

    event_loop->thread_id = pthread_self();
    event_loop->thread_name = thread_name == NULL ? "thread_main" : thread_name;
    pthread_mutex_init(&(event_loop->mutex), NULL);

    return NULL;
}

int event_loop_run(event_loop_t *event_loop)
{
    assert(event_loop != NULL);
    // 比较线程id是否相等，如果不等就是错误
    if (event_loop->thread_id != pthread_self())
    {
        printf("thread_id of event loop is not equals to pthread_self()");
        return -1;
    }
    dispatcher_t *dispatcher = event_loop->dispatcher;
    while (!(event_loop->is_quit))
    {
        dispatcher->dispatch(event_loop, 2000);
    }
    return 0;
}

int event_loop_add_task(event_loop_t *event_loop, channel_task_t *channel_task)
{
    return 0;
}

int event_loop_active(event_loop_t *event_loop, int fd, int type)
{
    if (event_loop == NULL)
    {
        perror("event_loop_active");
        return -1;
    }

    if (fd < 0)
    {
        perror("event_loop_active fd illegal");
        return -1;
    }

    channel_t *channel = event_loop->channel_map->list[fd];
    
    //根据传入的type判断执行读还是写回调
    if (type == FD_EVENT_READ_EVENT)
        channel->read_callback(channel->args);

    if (type == FD_EVENT_WRITE_EVENT)
        channel->write_callback(channel->args);

    return 0;
}