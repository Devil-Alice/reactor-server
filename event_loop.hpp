#pragma once
#include "dispatcher.hpp"
#include <thread>
#include <queue>
#include <map>
#include <mutex>
#include "channel.hpp"

class Dispatcher;

enum class CHANNEL_TASK_TYPE : char
{
    ADD,
    REMOVE,
    MDOIFY
};

class Channel_task
{
public:
    Channel *channel;
    enum CHANNEL_TASK_TYPE task_type;
};

/**
 * @brief event_loop结构体是用于管理文件描述符事件的结构
 */
class Event_loop
{
private:
    // 是否结束的标志位
    bool is_quit;
    // 这里的dispatcher是多态：父类指针指向子类对象
    Dispatcher *dispatcher;
    // void *dispatcher_data;

    // 任务队列，存放channel_task
    std::queue<Channel_task *> task_queue;

    // chaannelmap树文件描述符和对应的毁掉函数的映射
    std::map<int, Channel *> channel_map;

    // 线程信息
    std::thread::id thread_id;
    std::string thread_name;
    std::mutex mutex;
    pthread_cond_t cond;

    // socketpair是用于主线程和子线程通信的，是我们手动创建的一对fd
    // 由于select、poll、epoll在无数据的时候会阻塞，所以此时主线程需要通过socketpair向子线程发送数据，唤醒子线程
    // 规定socket_pair[0]发送数据，socket_pair[1]接收数据
    int socket_pair[2];

public:
    Event_loop(std::string thread_name);
    ~Event_loop();
    std::thread::id get_thread_id() { return thread_id; }
    std::string get_thread_name() { return thread_name; }

    int run();
    void add_task(Channel *channel, CHANNEL_TASK_TYPE task_type);
    void add_channel(Channel *channel);
    void remove_channel(Channel *channel);
    void modify_channel(Channel *channel);
    void manage_tasks();
    int process_event(int fd, CHANNEL_EVENT type);
    void wakeup();
    int socket_pair_read();
};

// 为event_loop_process_event创建的结构体，用于线程函数参数的传入
class Arg_event_data
{
public:
    Event_loop *event_loop;
    int fd;
    CHANNEL_EVENT type;
};

void *threadfunc_event_loop_process_event(void *arg_event_data);
// 这个函数的args用来存放event_loop_t *类型
