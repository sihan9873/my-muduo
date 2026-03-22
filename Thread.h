#pragma once

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

#include "noncopyable.h"

class Thread : noncopyable
{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func, const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool started() const { return started_; }
    // 在CurrentThread.cc中调用syscall(SYS_gettid)获取的真实的tid
    pid_t tid() const { return tid_; }
    const std::string &name() const { return name_; }

    static int numCreated() { return numCreated_; }

private:
    void setDefaultName();

    // 一个Thread对象,记录的是一个新线程的详细信息...
    bool started_;
    bool joined_;
    // 不能直接这么写,否则会和linux的pthread_create()一样,直接启动线程
    // std::thread thread_;
    std::shared_ptr<std::thread> thread_;
    pid_t tid_;
    ThreadFunc func_; // 存储线程函数
    std::string name_;
    static std::atomic_int numCreated_; // 创建的线程数
};
