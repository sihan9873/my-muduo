#include "EpollPoller.h"
#include "Logger.h"
#include "Channel.h"
#include <errno.h>
#include <unistd.h>
#include <string.h>

// 状态表示常量
// channel没有添加到Poller里
const int kNew = -1; // channel成员变量index_初始值为-1,index_表示channel在epoll中的状态
// channel已经添加到Poller里
const int kAdded = 1;
// channel已经从Poller中删除
const int kDeleted = 2;

EpollPoller::EpollPoller(EventLoop *loop)
    : Poller(loop), epollfd_(::epoll_create1(EPOLL_CLOEXEC)), events_(kInitEventListSize) // vector<epoll_event>
{
    if (epollfd_ < 0)
    {
        LOG_FATAL("epoll_create error:%d \n", errno);
    }
}

EpollPoller::~EpollPoller()
{
    ::close(epollfd_);
}

// EventLoop调用EpollPoller::poll()
// EventLoop会创建并传入ChannelList*
// poll通过epoll_wait监听发生的事件,并打包给ChannelList*返回给EventLoop
Timestamp EpollPoller::poll(int timeoutMs, EpollPoller::ChannelList *activeChannels)
{
    // 实际上应该用LOG_DEBUG
    // 因为poll是高频调用的函数,LOG_INFO会影响性能
    LOG_INFO("func=%s => fd total count:%ld\n", __FUNCTION__, channels_.size());
    // 监听两类fd,一种是client的fd,一种是wakeupfd
    int numEvents = ::epoll_wait(epollfd_, &*events_.begin(), static_cast<int>(events_.size()), timeoutMs);
    // 调用完epoll_wait立即保存errno,因为多个线程在运行EventLoop,防止丢失
    int savedErrno = errno;
    Timestamp now(Timestamp::now());
    if (numEvents > 0)
    {
        LOG_INFO("%d events happened \n", numEvents);
        fillActiveChannels(numEvents, activeChannels);
        // 扩容
        // epoll_wait会按照传入的最大可读范围写入事件
        // 若实际写的数量等于容器数量,则说明写满了,需要扩容
        if (numEvents == events_.size())
        {
            events_.resize(events_.size() * 2);
        }
    }
    else if (numEvents == 0)
    {
        LOG_DEBUG("func=%s timeout! \n", __FUNCTION__);
    }
    else
    {
        // 如果是savedErrno = EINTR
        // 说明相应外部中断,否则发生error
        if (savedErrno != EINTR)
        {
            // 需要把savedErrno赋值回去是因为项目源码在LOG_ERROR中访问的是errno
            // 此处是仿照源码处理
            errno = savedErrno;
            LOG_ERROR("EpollPoller::poll() err!\n");
        }
    }
    return now;
}

/**
 * EventLoop中包含ChannelList和Poller
 * Poller中包含ChannelMap<fd,channel*>
 * EventLoop中包含的是所有的channel
 * Poller的ChannelMap中包含的是在epoll中注册过的channel
 * ChannelList >= ChannelMap
 */
void EpollPoller::updateCahnnel(Channel *channel)
{
    const int index = channel->index();
    LOG_INFO("func=%s fd=%d events=%d index=%d \n", __FUNCTION__, channel->fd(), channel->events(), channel->index());
    if (index == kNew || index == kDeleted)
    {
        // channel没注册过或已经从Poller中删掉
        if (index == kNew)
        {
            int fd = channel->fd();
            // 登记到Poller中的ChannelMap
            channels_[fd] = channel;
        }
        // 更新channel在poller中的状态
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD, channel);
    }
    else
    {
        // channel已经在poller中注册过了
        int fd = channel->fd();
        if (channel->isNoneEvent())
        {
            // 这个channel没有感兴趣的事件了,也就是说,它不需要被poller继续监听了
            update(EPOLL_CTL_DEL, channel);
            channel->set_index(kDeleted);
        }
        else
        {
            // 还有感兴趣的事件,可能是有事件更新
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void EpollPoller::removeCahnnel(Channel *channel)
{
    // 从poller中删除channel
    int fd = channel->fd();
    channels_.erase(fd);

    LOG_INFO("func=%s fd=%d\n", __FUNCTION__, channel->fd());

    int index = channel->index();
    if (index == kAdded)
    {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->set_index(kNew);
}

void EpollPoller::fillActiveChannels(int numEvents, ChannelList *activeChannels) const
{
    for (int i = 0; i < numEvents; ++i)
    {
        Channel *channel = static_cast<Channel *>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        // EventLoop拿到它的poller给它返回的所有发生事件的channel列表
        activeChannels->push_back(channel);
    }
}

void EpollPoller::update(int operation, Channel *channel)
{
    int fd = channel->fd();
    // epoll_ctl(int epfd, int op, int fd, struct epoll_event *event);
    // 创建epoll_event
    epoll_event event;
    memset(&event, 0, sizeof event);
    event.events = channel->events();
    event.data.fd = fd;
    event.data.ptr = channel;

    if (::epoll_ctl(epollfd_, operation, fd, &event) < 0)
    {
        if (operation == EPOLL_CTL_DEL)
        {
            LOG_ERROR("epoll_ctl del error: %d\n", errno);
        }
        else
        {
            LOG_FATAL("epoll_ctl add/mod error:%d\n", errno);
        }
    }
}
