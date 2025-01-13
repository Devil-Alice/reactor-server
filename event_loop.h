#pragma once
#include "dispatcher.h"
#include "linked_list.h"
#include "channel_map.h"

extern dispatcher_t epoll_dispatcher;

enum CHANNEL_TASK_TYPE{CHANNEL_TASK_TYPE_ADD, CHANNEL_TASK_TYPE_REMOVE, CHANNEL_TASK_TYPE_MDOIFY};

typedef struct channel_task
{
    channel_t *channel;
    int task_type;
} channel_task_t;

/**
 * @brief event_loop结构体是用于管理文件描述符事件的结构
 */
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

    //socketpair是用于主线程和子线程通信的，是我们手动创建的一对fd
    //由于select、poll、epoll在无数据的时候会阻塞，所以此时主线程需要通过socketpair向子线程发送数据，唤醒子线程
    //规定socket_pair[0]发送数据，socket_pair[1]接收数据
    int socket_pair[2];


} event_loop_t;


event_loop_t* event_loop_create_main(void);
event_loop_t* event_loop_create(char* thread_name);

int event_loop_run(event_loop_t *event_loop);
int event_loop_add_task(event_loop_t *event_loop, channel_t *channel, int task_type);
int event_loop_add_channel(event_loop_t *event_loop, channel_t *channel);
int event_loop_remove_channel(event_loop_t *event_loop, channel_t *channel);
int event_loop_destroy_channel(event_loop_t *event_loop, channel_t *channel);
int event_loop_modify_channel(event_loop_t *event_loop, channel_t *channel);
int event_loop_manage_tasks(event_loop_t *event_loop);
int event_loop_process_event(event_loop_t *event_loop, int fd, int type);
int event_loop_wakeup_event(event_loop_t *event_loop);
//这个函数的args用来存放event_loop_t *类型
int event_loop_socket_pair_read(void *args);
