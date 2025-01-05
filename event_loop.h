#pragma once
#include "dispatcher.h"
#include "linked_list.h"
#include "channel_map.h"

extern dispatcher_t epoll_dispatcher;

enum channel_task_type{CHANNEL_TASK_ADD, CHANNEL_TASK_DEL, CHANNEL_TASK_MOD};

typedef struct channel_task
{
    channel_t channel;
    int task_type;
} channel_task_t;

typedef struct event_loop
{
    //是否结束的标志位
    bool is_quit;
    dispatcher_t *dispatcher;
    void *dispatcher_data;

    //任务队列，存放channel_task
    linked_list_t task_queue;

    //chaannelmap树文件描述符和对应的毁掉函数的映射
    channel_map_t *channel_map;

    //线程信息
    pthread_t thread_id;
    char* thread_name;
    pthread_mutex_t mutex;
    pthread_cond_t cond;


} event_loop_t;


event_loop_t* event_loop_create_main(void);
event_loop_t* event_loop_create(char* thread_name);

int event_loop_run(event_loop_t *event_loop);
int event_loop_add_task(event_loop_t *event_loop, channel_task_t *channel_task);
int event_loop_active(event_loop_t *event_loop, int fd, int type);