#include "Poller.h"
#include <stdlib.h>
// #include "EpollPoller.h"
// #include "PollPoller.h"

Poller *Poller::newDefaultPoller(EventLoop *loop)
{
    if (::getenv("MUDUO_USE_POLL"))
    {
        // 生成poll的实例
        return nullptr;
    }
    else
    {
        // return new EpollPoller();
        return nullptr;
    }
}
