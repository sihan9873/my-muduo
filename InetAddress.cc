#include "InetAddress.h"
#include <strings.h>
#include <string.h>

// 封装socket的ip端口
// 声明的默认参数只能出现一次
InetAddress::InetAddress(uint16_t port, std::string ip)
{
    bzero(&addr_, sizeof addr_);                   // 清零
    addr_.sin_family = AF_INET;                    // 指定套接字和 IP 地址家族
    addr_.sin_port = htons(port);                  // 本地字节序转换成网络字节序,主要是大端小端要对齐,网络是大端
    addr_.sin_addr.s_addr = inet_addr(ip.c_str()); // 传入ip
}

std::string InetAddress::toIp() const
{
    // 从addr_中读出ip地址,并转成本地字节序
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    return buf;
}

std::string InetAddress::toIpPort() const
{
    // ip:port格式
    char buf[64] = {0};
    ::inet_ntop(AF_INET, &addr_.sin_addr, buf, sizeof buf);
    size_t end = strlen(buf);
    uint16_t port = ntohs(addr_.sin_port);
    sprintf(buf + end, ":%u", port);
    return buf;
}

uint16_t InetAddress::toPort() const
{
    return ntohs(addr_.sin_port);
}

#include <iostream>
int main()
{
    InetAddress addr(8080);
    std::cout << addr.toIpPort() << std::endl;
    return 0;
}
