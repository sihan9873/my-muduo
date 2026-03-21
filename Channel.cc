#include "Channel.h"
#include "EventLoop.h"
#include "Logger.h"
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop, int fd)
    : loop_(loop), fd_(fd), events_(0), revents_(0), index_(-1), tied_(false)
{
}

Channel::~Channel() {}

// channel的tie方法什么时候调用过?之后再补上
void Channel::tie(const std::shared_ptr<void> &obj)
{
    tie_ = obj;
    tied_ = true;
}

// 当改变channel所表示fd的events事件后,update负责在poller里面更改fd相应的事件epoll_ctl
// EventLoop => Channel List + Poller
// Channel和Poller是"兄弟关系",因此需要通过EventLoop来进行一些操作
void Channel::update()
{
    // 通过Channel所属的EventLoop,调用poller的相应方法,注册fd的events事件
    // add code...
    loop_->updateChannel(this);
}

// 在channel所属的EventLoop中,把当前的channel删除掉
void Channel::remove()
{
    // add code...
    loop_->removeChannel(this);
}

// fd得到poller通知后,处理事件的函数
void Channel::HandleEvent(Timestamp receiveTime)
{
    LOG_INFO("channel handleEvent revents: %d", revents_);

    // 用智能指针绑定当前channel
    if (tied_)
    {
        std::shared_ptr<void> guard = tie_.lock(); // 把weak_ptr提升成shared_ptr
        if (guard)
        {
            handleEventWithGuard(receiveTime);
        }
    }
    else
    {
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的channel发生的具体事件,由channel负责调用具体的回调函数
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    // 出大问题的情况
    if ((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN))
    {
        if (closeCallback_)
        {
            // closeCallback_非空则执行
            closeCallback_();
        }
    }

    // 发生错误
    if (revents_ & EPOLLERR)
    {
        if (errorCallback_)
        {
            errorCallback_();
        }
    }

    // 可读事件
    if (revents_ & (EPOLLIN | EPOLLPRI))
    {
        if (readCallback_)
        {
            readCallback_(receiveTime);
        }
    }

    // 可写事件
    if (revents_ & EPOLLOUT)
    {
        if (writeCallback_)
        {
            writeCallback_();
        }
    }
}
