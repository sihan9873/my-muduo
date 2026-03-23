#include "Socket.h"
#include "InetAddress.h"
#include "Logger.h"
#include <strings.h>

Socket::~Socket()
{
    close(sockfd_);
}

void Socket::bindAddress(const InetAddress &localaddr)
{
    if (::bind(sockfd_, (sockaddr *)localaddr.getSockAddr(), sizeof(sockaddr_in)) != 0)
    {
        LOG_FATAL("bind sockfd: %d fail \n", sockfd_);
    }
}

void Socket::listen()
{
    // 记得加上全局作用域"::",避免冲突
    if (::listen(sockfd_, 1024) != 0)
    {
        LOG_FATAL("listen sockfd: %d fail \n", sockfd_);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    sockaddr_in addr;
    socklen_t len;
    bzero(&addr, sizeof addr);
    int connfd = ::accept(sockfd_, (sockaddr *)&addr, &len);
    if (connfd >= 0)
    {
        // 将内核填充的客户端地址，存入InetAddress对象传出
        peeraddr->setSockAddr(addr);
    }
    // 返回新连接fd
    return connfd;
}

// 半关闭TCP连接
void Socket::shutdownWrite()
{
    if (::shutdown(sockfd_, SHUT_WR) < 0)
    {
        LOG_ERROR("shutdownWrite error");
    }
}

// 封装一些底层设置的接口调用
void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY, &optval, sizeof optval);
}

void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE, &optval, sizeof optval);
}