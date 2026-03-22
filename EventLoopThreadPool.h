#pragma once

#include "noncopyable.h"

#include <functional>
#include <string>
#include <vector>
#include <memory>

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool : noncopyable
{
public:
    // 线程初始化回调,是从上层传递下来的
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    EventLoopThreadPool(EventLoop *baseLoop, const std::string &nameArg);
    ~EventLoopThreadPool();

    void setThreadNum(int numThreads) { numThreads_ = numThreads; }

    // 开启事件循环线程
    void start(const ThreadInitCallback &cb = ThreadInitCallback());

    // 如果工作在多线程中,baseLoop_默认以轮询的方式分配channel给subLoop
    EventLoop *getNextLoop();

    std::vector<EventLoop *> getAllLoops();

    bool started() const { return started_; }
    const std::string name() const { return name_; }

private:
    // Server中的mainReactor,如果不调用setThreadNum()的花,就只有这个线程,同时负责accept和handle
    EventLoop *baseLoop_;
    std::string name_;
    bool started_;
    int numThreads_;
    int next_;
    // 所有subReactor的线程
    std::vector<std::unique_ptr<EventLoopThread>> threads_;
    // 所有的subReactor
    // Don't delete loop, it's stack variable
    std::vector<EventLoop *> loops_;
};