#pragma once

#include "Poller.h"
#include "Timestamp.h"
#include <vector>
#include <sys/epoll.h>

class Channel;

/**
 * epoll的使用
 * epoll_create => Poller的构造函数中封装
 * epoll_ctl => add/mod/del => Poller的update函数封装
 * epoll_wait => Poller的poll函数封装
 */

// 实现基类的虚函数
class EpollPoller : public Poller
{
public:
    EpollPoller(EventLoop *loop);
    ~EpollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeoutMs, ChannelList *activeChannels) override;
    void updateCahnnel(Channel *channel) override;
    void removeCahnnel(Channel *channel) override;

private:
    // EventList的初始长度
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents, ChannelList *activeChannels) const;
    // 更新channel通道
    // 实际即为epoll_ctl()的封装
    void update(int operation, Channel *channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;      // 多路监听fd
    EventList events_; // 关注的事件
};
