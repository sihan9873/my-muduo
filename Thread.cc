#include "Thread.h"
#include "CurrentThread.h"

#include <semaphore.h>

// 静态变量需要在类外单独初始化,千万别忘了
// atomic类型必须用构造方式初始化，而非赋值
std::atomic_int Thread::numCreated_{0};

Thread::Thread(ThreadFunc func, const std::string &name) : started_(false),
                                                           joined_(false),
                                                           tid_(0),
                                                           func_(std::move(func)),
                                                           name_(name)

{
    setDefaultName();
}

Thread::~Thread()
{
    if (started_ && !joined_)
    {
        // detach()把线程设置成分离线程(也就是守护线程)
        // 当主线程结束,守护线程也会自动结束,不用担心产生孤儿线程泄露内核资源
        // 但detach()和pthread_join()不能同时执行=>当前线程要么是工作线程,要么是分离线程
        // pthread_join():让主线程（或调用线程）等待指定的子线程结束，并可获取子线程的退出状态。
        // pthread_join()作用在普通的工作线程上(而不是分离线程)
        thread_->detach();
    }
}

// 线程启动
// 一个Thread对象,记录的是一个新线程的详细信息
void Thread::start()
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0);

    // 开启新线程,专门执行线程函数
    // 创建新的线程对象.给他传递线程函数(lamda函数)
    // lamda表达式以引用的方式接收外部的Thread对象
    thread_ = std::shared_ptr<std::thread>(new std::thread([&]() { // 子线程的执行逻辑
        // 获取线程的tid值
        tid_ = CurrentThread::tid();
        // 给信号量资源+1
        sem_post(&sem);
        // 开启新线程,专门执行该线程函数
        func_();
    }));

    // 母线程的执行逻辑
    // 此处必须等待获取上面新创建的线程的tid值
    // 用信号量处理,sem没有资源的时候,此处会阻塞等待
    sem_wait(&sem);
    // 用完信号量要销毁，避免资源泄漏
    sem_destroy(&sem);
}

void Thread::join()
{
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName()
{
    int num = ++numCreated_;
    if (name_.empty())
    {
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);
        name_ = buf;
    }
}