#pragma once

#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#include <netinet/tcp.h>
#include "noncopyable.h"

class InetAddress;

// 对于sokcet的简单封装
class Socket : noncopyable
{
public:
    explicit Socket(int sockfd) : sockfd_(sockfd) {}
    ~Socket();

    int fd() const { return sockfd_; }
    void bindAddress(const InetAddress &localaddr);
    void listen();
    int accept(InetAddress *peeraddr);

    // 半关闭TCP连接
    void shutdownWrite();

    // 封装一些底层设置的接口调用
    void setTcpNoDelay(bool on); // 不进行TCP缓冲
    void setReuseAddr(bool on);
    void setReusePort(bool on);
    void setKeepAlive(bool on); // 保持长连接

private:
    const int sockfd_;
};