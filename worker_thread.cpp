#include "worker_thread.hpp"
#include "epoll_dispatcher.hpp"
#include "log.h"

Worker_thread::Worker_thread(std::string name)
{
    thread = nullptr;
    id = std::thread::id();
    this->name = name;
    event_loop = nullptr;
}

Worker_thread::~Worker_thread()
{
    if (thread != nullptr)
        delete thread;
    if (event_loop != nullptr)
        delete event_loop;
}

void Worker_thread::run()
{
    // 注意：这里创建好子线程后，并不一定会立刻执行
    // task_func这个函数是主线程执行的，而eventloop时在子线程中初始化的
    // 也就意味着线程函数中对于eventloop的初始化还未完成，此时主线程如果访问eventloop就会报错
    // 所以，需要使用条件变量cond来确保eventloop正确初始化完成
    // pthread_create(id, NULL, threadfunc_event_loop_run, this);
    thread = new std::thread(&Worker_thread::task_func, this, this);

    // 在这里等待eventloop初始化完毕

    std::unique_lock<std::mutex> locker(mutex);
    while (event_loop == NULL)
    {
        // cpp中的 条件变量使用和c中有很大区别
        // 首先cond.wait需要传入一个unique_lock
        // 而unique_lock locker需要包装我们需要使用的mutex
        // cpp中的cond.wait会自动对这个locker进行加锁解锁的操作，也就省去我们手动对mutex加锁解锁的代码了
        cond.wait(locker);
    }

    // 此线程其他操作这时候就可以正常访问eventloop了
    // 例如：主线程（也就是主反应堆）在接收到了一个链接请求后，会访问分配给子线程的反应堆（eventloop）
}

void Worker_thread::task_func(void *args)
{
    mutex.lock();
    event_loop = new Event_loop(name);
    mutex.unlock();
    cond.notify_one();

    event_loop->run();
}
