#pragma once
#include "noncopyable.h"

class InetAddress;

// Encapsulate the socket_fd into that object
class Socket: noncopyable{
public:
    explicit Socket(int sockfd)
        :sockfd_(sockfd)
    {}

    ~Socket();

    int fd() const {return sockfd_;}
    void bindAddress(const InetAddress &localaddr);  // 绑定本地地址和端口
    void listen();  // 开始监听连接请求
    int accept(InetAddress *peeraddr);  // 接受客户端连接，返回新的socket文件描述符

    void shutdownWrite();  // 关闭写操作

    void setTcpNoDelay(bool on);  // 设置是否禁用TCP Nagle算法
    void setReuseAddr(bool on);  // 设置是否重用本地地址
    void setReusePort(bool on);  // 设置是否重用端口
    void setKeepAlive(bool on);  // 设置是否启用TCP保活机制

private:
    const int sockfd_;
};

