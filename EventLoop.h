#pragma once

#include <functional>
#include <vector>
#include <atomic>
#include <memory>
#include <mutex>
#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

class Channel;
class Poller;

// 事件循环类
// 主要包含了个大模块Channel Poller(epoll的抽象)
class EventLoop : noncopyable
{
public:
    // 定义回调类型
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const { return pollReturnTime_; }

    // 在当前loop中执行回调
    void runInLoop(Functor cb);
    // 把cb放入队列中,唤醒loop所在的线程,执行cb
    void queueInLoop(Functor cb);

    // 唤醒loop所在的线程
    void wakeup();

    // channel通过loop的updateChannel(),间接调用poller的updateChannel()
    // 来更改poller上channel相关的fd的事件
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    bool hasChannel(Channel *channel); // 用不着,但源码有,所以还是保留一下吧

    // 判断EventLoop对象是否在自己的线程里面
    bool isInLoopThread() const
    {
        // eventLoop的线程id == 当前线程的id
        // 若二者相等,可以正常执行回调
        // 若不是,需要queueInLoop
        return threadId_ == CurrentThread::tid();
    }

private:
    void handleRead();        // 处理wakeup,用于唤醒subReactor
    void doPendingFunctors(); // 执行回调

    using ChannelList = std::vector<Channel *>;

    // 控制loop事件循环是否继续的标识符
    std::atomic_bool looping_; // 原子操作,通过CAS实现的
    std::atomic_bool quit_;    // 标识退出loop循环

    // 记录当前loop所在线程id
    const pid_t threadId_;

    Timestamp pollReturnTime_; // poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    // mainReactpr给SubReactor分配新连接时采用的负载算法是轮询操作
    // 主要作用:
    // 当mainLoop获取一个新用户的channel,
    // 通过轮询算法选择一个subLoop,
    // 通过该成员唤醒subLoop处理channel
    // 用到了eventfd()这个系统调用,相当于线程间的通信机制,内核去notify线程
    int wakeupFd_;
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;

    // 标识当前loop是否有需要执行的回调操作
    std::atomic_bool callingPendingFunctors_;
    // 存储loop需要执行的所有回调操作
    std::vector<Functor> pendingFunctors_;
    // 互斥锁,用来保护上面vector容器的线程安全操作
    std::mutex mutex_;
};