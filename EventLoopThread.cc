#include "EventLoopThread.h"
#include "EventLoop.h"

EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
                                 const std::string &name)

    : loop_(nullptr),
      exiting_(false),
      thread_(std::bind(&EventLoopThread::threadFunc, this), name),
      // 类成员函数需要绑定当前对象,使之满足Thread类要求的回调函数类型
      // 此时创建了thread对象,但并未创建子线程,根据Thread.cc代码,thread_->start()时才真正创建线程,执行这里给线程注册的回调
      mutex_(),
      cond_(),
      callack_(cb)
{
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if (loop_ != nullptr)
    {
        // 线程退出时,记得要把与之关联的loop也退出了
        loop_->quit();
        // thread_需要等待底下的子线程结束
        thread_.join();
    }
}

/**
 * 执行逻辑:
 * 主线程startLoop()->启动子线程->阻塞在cond_.wait直到子线程初始化 loop_
 * 子线程执行threadFunc()->创建 EventLoop loop->初始化 loop_ = &loop->唤醒主线程
 * 子线程执行的threadFunc()是在EventLoopThread的构造函数中注册到子线程的
 */

EventLoop *EventLoopThread::startLoop()
{
    // 启动循环,实现逻辑:
    thread_.start(); // 启动新线程,执行threadFunc()
    EventLoop *loop = nullptr;
    // 等待新线程创建好loop,通过条件变量通知此线程接收
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr)
        {
            cond_.wait(lock);
        }
        loop = loop_;
    }
    return loop;
}

// 此方法是在单独的新线程中运行的,在EventLoopThread()中被注册进新线程的回调函数,负责启动loop
void EventLoopThread::threadFunc()
{
    // 创建一个独立的eventloop,和这个新线程一一对应,即one loop per thread
    EventLoop loop;
    // 如果有ThreadInitCallback,则在此处执行
    if (callack_)
    {
        callack_(&loop);
    }

    {
        // 主线程(调用 startLoop())和子线程(执行 threadFunc())并发访问 loop_ 变量
        // 需要通过锁 + 条件变量保证线程安全和执行顺序。
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop;
        cond_.notify_one();
    }

    // EventLoop::loop() -> Poller::poll()
    // 子线程启动EventLoop对应loop()函数,启动其底层的Poller进行监听
    // 进入阻塞状态,开启监听
    loop.loop();
    // poller返回,监听结束,事件循环结束,服务器进入关闭流程
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}
