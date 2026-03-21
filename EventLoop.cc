#include "EventLoop.h"
#include "Logger.h"
#include "Poller.h"
#include "Channel.h"
#include <sys/eventfd.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <memory>

// 防止一个线程创建多个EventLoop
__thread EventLoop *t_loopInThisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupFd,用来notify唤醒subReactor处理新来的channel
int createEventfd()
{
    // 用来通知休眠的EventLoop起来处理新连接的用户channel
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);
    if (evtfd < 0)
    {
        LOG_FATAL("eventfd error:%d \n", errno);
    }
    return evtfd;
}

EventLoop::EventLoop() : looping_(false),
                         quit_(false),
                         callingPendingFunctors_(false),
                         threadId_(CurrentThread::tid()),
                         poller_(Poller::newDefaultPoller(this)),
                         wakeupFd_(createEventfd()),
                         wakeupChannel_(new Channel(this, wakeupFd_)) // 每个subReactor都监听wakeupChannel,所以mainReactor可以通过wakeupFd_去notify对应的subReactor
{
    LOG_DEBUG("EventLoop created %p in thread %d \n", this, threadId_);
    if (t_loopInThisThread)
    {
        LOG_FATAL("Another EventLoop %p exists in this thread %d \n", t_loopInThisThread, threadId_);
    }
    else
    {
        t_loopInThisThread = this;
    }
    // 设置wakeupFd_的事件类型以及事件发生后的回调操作
    // wakefd用于唤醒subReactor,也就是loop,loop阻塞在epoll_wait,唤醒后返回
    // loop就可以处理新的channel
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead, this));
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop()
{
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInThisThread = nullptr;
}

void EventLoop::handleRead()
{
    // 这段逻辑并不重要
    // 关键在于通过触发事件,来唤醒监听wakefd_及其channel的subReactor(Poller)
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::handleRead() read %lu bytes instead of 8", n);
    }
}

void EventLoop::loop()
{
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n", this);

    while (!quit_)
    {
        activeChannels_.clear();
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        for (Channel *channel : activeChannels_)
        {
            // Poller监听那些channel发生事件了,然后上报给EventLoop
            // EventLoop再通知channel处理相应的事件
            channel->HandleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调函数,比如mainLoop派发的新channel需要处理
        /**
         * IO线程(mainReactor)=>accept fd=>channel=>subLoop
         * mainLoop事先注册一个回调cb(需要subLoop执行) ->dependingFunctor
         * mainLoop通过wakeup唤醒subLoop去执行mainLoop注册好的回调
         */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stopp looping. \n", this);
    looping_ = false;
}

// 退出事件循环:
// 1.loop再自己的线程中调用quit
// 2.在非loop得线程中,调用loop的quit
//  比如在一个subLoop中调用了mainLoop的quit
//  比如在一个工作线程调用了loop的quit
void EventLoop::quit()
{
    quit_ = true;
    if (!isInLoopThread())
    {
        // 2.再其他线程中调用的quit
        // 把这个loop唤醒,这个线程从poll()中返回后,quit_状态已经改变,循环自然终止
        // 此处功能可以扩展:
        //      在mainLoop和subLoop之间设置线程安全的队列来缓冲一下
        //      mainLoop可以把新连接的channel放入队列,subLoop从中取出(生产者消费者模型)
        //      此处采用的是轮询算法实现,是通过eventfd()的直接通信
        wakeup();
    }
}

// 在当前loop中执行回调
void EventLoop::runInLoop(Functor cb)
{
    if (isInLoopThread())
    {
        // 在当前loop线程中执行cb
        cb();
    }
    else
    {
        // 在非当前loop的线程中执行cb,就需要唤醒loop所在的线程,再执行
        queueInLoop(cb);
    }
}

// 把cb放入队列中,唤醒loop所在的线程,执行cb
void EventLoop::queueInLoop(Functor cb)
{
    {
        std::unique_lock<std::mutex> lock(mutex_);
        pendingFunctors_.emplace_back(cb);
    }
    // 1.当前线程不是loop对应的线程
    // 唤醒相应的、需要执行上面回调函数的loop的线程
    // 2.callingPendingFunctors_:
    // 当前线程是loop对应的线程,但是loop此时在执行之前的回调,写完后会重新进入poll阻塞,此时loop又要写入新的回调
    if (!isInLoopThread() || callingPendingFunctors_)
    {
        wakeup();
    }
}

// 唤醒loop所在的线程
// 向wakeupfd写数据=>触发wakefd的写事件=>loop的poller监听到事件,从poll()返回=>唤醒loop所在的线程
// 实现逻辑:向wakeupfd写数据,写啥无所谓
void EventLoop::wakeup()
{
    uint64_t one = 1;
    ssize_t n = write(wakeupFd_, &one, sizeof one);
    if (n != sizeof one)
    {
        LOG_ERROR("EventLoop::wakeup() writes %lu bytes instead of 8 \n", n);
    }
}

void EventLoop::updateChannel(Channel *channel)
{
    poller_->updateCahnnel(channel);
}

void EventLoop::removeChannel(Channel *channel)
{
    poller_->removeCahnnel(channel);
}

bool EventLoop::hasChannel(Channel *channel)
{
    return poller_->hasChannel(channel);
}

// 执行回调
void EventLoop::doPendingFunctors()
{
    std::vector<Functor> functors;
    callingPendingFunctors_ = true;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        // 为了一次性取出需要毁掉的函数
        // 避免长期占用共享空间,导致阻塞,增加时延
        // 比如,正在一个个处理pendingFunctors_中的回调函数时,mainLoop需要往subLoop中注册新的channel,就需要阻塞等待
        functors.swap(pendingFunctors_);
    }
    for (const Functor &functor : functors)
    {
        // 执行当前loop需要执行的回调操作
        functor();
    }
    callingPendingFunctors_ = false;
}
