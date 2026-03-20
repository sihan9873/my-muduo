#pragma once

#include <functional>
#include <memory>
#include "noncopyable.h"
#include "Timestamp.h"

// 前置声明参数中用到的类
class EventLoop;

// Channel类理解为通道
// 封装了sockfd及其感兴趣的event,如EPOLLIN,EPOLLOUT事件
// 还绑定了poller返回的具体事件revent

// 理清楚EventLoop,Channel,Poller之间的关系:
// EventLoop包含后面两个,又称之为Reactor,Demultiplex,不断的事件循环和监听,one loop per thread
// poller,封装并调用epoll,即多路复用I/O
// channel,封装fd和fd关注的事件
class Channel : noncopyable
{
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *loop, int fd);
    ~Channel();

    // fd得到poller通知以后,处理事件,调用相应的回调函数
    void HandleEvent(Timestamp receiveTime);

    // Channel不知道具体做什么样的回调,只是根据情况被动地执行注册好的回调函数
    // 回调函数都是私有的,所以要提供对外设置的接口
    // 二者都是左值,有名字有内存,且function类型比较大,用move直接转成右值赋值,开销更小
    void setReadCallback(ReadEventCallback cb) { readCallback_ = std::move(cb); }
    void setWriteCallback(EventCallback cb) { writeCallback_ = std::move(cb); }
    void setCloseCallback(EventCallback cb) { closeCallback_ = std::move(cb); }
    void setErrorCallback(EventCallback cb) { errorCallback_ = std::move(cb); }

    // 防止Channel被手动remove掉后,channel还在执行回调函数
    void tie(const std::shared_ptr<void> &);

    int fd() const { return fd_; }
    int events() const { return events_; }
    // poller负责监听事件,channel提供对外的接口,以便于poller设置
    int set_revents(int revt) { revents_ = revt; }

    // 设置fd相应的事件状态,再通过epoll执行真正的epoll_ctl()调用
    void enableReading()
    {
        events_ |= kReadEvent;
        update();
    }
    void disableReading()
    {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting()
    {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting()
    {
        events_ &= ~kWriteEvent;
        update();
    }
    void disableAll()
    {
        events_ = kNoneEvent;
        update();
    }

    // 返回fd当前的事件状态
    bool isNoneEvent() const { return events_ == kNoneEvent; }
    bool isWriting() const { return events_ & kWriteEvent; }
    bool isReading() const { return events_ & kReadEvent; }

    int index() { return index_; }
    void set_index(int idx) { index_ = idx; }

    // 当前channel所属的EventLoop
    EventLoop *ownerLoop() { return loop_; }

private:
    // 在前面的使能函数中调用
    // 调用epoll_ctl,在epoll中更新fd关注的事件
    void update();
    // handleEvent()的底层调用
    void handleEventWithGuard(Timestamp receiveTime);

    // fd状态信息:没感兴趣的事件/有读事件/有写事件
    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // Poller监听的fd对象
    int events_;      // 注册fd感兴趣的事件
    int revents_;     // Poller返回的fd具体发生的事件
    int index_;

    // 跨线程监听对象生存状态
    // 防止手动remove Channel之后,还在使用Channel
    // shared_ptr和weak_ptr搭配使用
    // 解决只用shared_ptr的循环引用问题
    // 多线程用weak_ptr监听对象生存状态,使用时把weak_ptr提升成shared_ptr
    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为Channel中能够获知fd具体发生的事件revents,所以它负责调用对应的回调
    // 触发回调不是Channel决定的,是用户指定的,通过接口传递给Channel,Channel负责调用
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};
