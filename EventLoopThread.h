#pragma once

#include <functional>
#include <string>
#include <mutex>
#include <condition_variable>
#include <memory>

#include "noncopyable.h"
#include "Thread.h"

class EventLoop;

// EventLoopThread绑定了EventLoop和Thread
// 让这个loop在对应的thread上运行
// 换句话说,就是在这个thread里创建一个loop
// muduo库的核心思想:one loop per thread
class EventLoopThread : noncopyable
{
public:
    // 线程初始化回调,是从上层传递下来的
    using ThreadInitCallback = std::function<void(EventLoop *)>;
    // 在thread中创建loop的时候,可以传入这个回调对loop进行初始化,当然如果不传入的话,就默认调用线程初始化回调函数
    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(), const std::string &name = std::string());
    ~EventLoopThread();

    EventLoop *startLoop();

private:
    // 线程函数,实际创建loop
    void threadFunc();

    // 使用与互斥锁相关
    EventLoop *loop_;
    bool exiting_; // 是否退出循环
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_;
    ThreadInitCallback callack_;
};
