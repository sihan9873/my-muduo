#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include <vector>
#include <unordered_map>

class Channel;
class EventLoop;

// muduo库中多路事件分发器的核心IO服用模块
// 抽象类
// 以便于再实际调用中适配不同的派生类
// 实现各种类型的IO多路复用:select,poll,epoll
class Poller : noncopyable
{
public:
    using ChannelList = std::vector<Channel *>;

    Poller(EventLoop *loop);
    // 虚析构函数非常重要!
    // 如果你通过基类指针指向派生类对
    // 并在销毁时没有使用虚析构函数
    // 程序就会发生定义的未行为（Undefined Behavior）
    // 最直接的表现就是派生类的资源没被释放,比如派生类特有的成员变量
    virtual ~Poller();

    // 给所有IO复用保留统一的接口
    virtual Timestamp poll(int timeoutMs, ChannelList *activeChannels) = 0;
    virtual void updateCahnnel(Channel *channel) = 0;
    virtual void removeCahnnel(Channel *channel) = 0;

    // 判断参数channel是否在当前Poller中
    bool hasChannel(Channel *channel) const;

    // EventLoop可以通过该接口获取默认的IO复用的具体实现
    // 学习重点:为什么不把这个函数写在Poller.cc中?
    // 这个函数需要生成一个具体的IO复用对象,并且返回
    // 这说明这个函数需要引入相关头文件:PollPoller.h,EpollPoller
    // 这种在基类中引入派生类的写法非常不好
    // 写在单独的文件中比较合适
    static Poller *newDefaultPoller(EventLoop *loop);

protected:
    // map:[fd,fd对应的channel],可以通过fd快算查询对应的channel
    using ChannelMap = std::unordered_map<int, Channel *>;
    ChannelMap channels_;

private:
    // 定义Poller所属的事件循环EventLoop
    EventLoop *ownerLoop_;
};
