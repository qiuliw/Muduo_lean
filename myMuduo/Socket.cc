#include "Socket.h"
#include "InetAddress.h"
#include "Logger.h"

#include <asm-generic/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <fcntl.h>

Socket::~Socket()
{
    close(sockfd_); 
}
void Socket::bindAddress(const InetAddress &localaddr)
{
    if(0 != ::bind(sockfd_,(sockaddr*)localaddr.getSockAddr(),sizeof(sockaddr_in))){
        LOG_FATAL("bind sockfd:%d fail \n",sockfd_);
    }
}

void Socket::listen()
{   
    if(0 != ::listen(sockfd_, 1024))
    {
        LOG_FATAL("listen sockfd:%d fail \n",sockfd_);
    }
}

int Socket::accept(InetAddress *peeraddr)
{
    /**
        1. accept参数不合法
        2. 对返回的connfd没有设置非阻塞
    */

    sockaddr_in addr;
    socklen_t len = sizeof addr;
    bzero(&addr, sizeof addr);
    int connfd = ::accept(sockfd_, (sockaddr*)&addr, &len);
    if(connfd >=0){
        peeraddr->setSockAddr(addr); // 将客户端的地址传出

        // 设置 connfd 为非阻塞模式
        int flags = fcntl(connfd, F_GETFL, 0); //  获取文件描述符的当前属性
        flags |= O_NONBLOCK; // 设置为非阻塞模式
        int  ret = fcntl(connfd, F_SETFL, flags); //  设置文件描述符的属性

        // close-on-exec 子进程不继承fd,fd自动释放。mainloop执行
        flags = fcntl(connfd, F_GETFD, 0); // 获取文件描述符的close-on-exec属性
        flags |= FD_CLOEXEC; // 设置文件描述符为 close-on-exec
        ret = fcntl(connfd, F_SETFD, flags); //  设置文件描述符的close-on-exec属性


    }
    return connfd;
}


void Socket::shutdownWrite() // 半关闭
{
    if(::shutdown(sockfd_, SHUT_WR) < 0){
        LOG_ERROR("shutdownWrite error");
    }
}

void Socket::setTcpNoDelay(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_
        , IPPROTO_TCP // 操作TCP层
        ,TCP_NODELAY // 要设置的选项名，用于禁止 Nagle 算法，使小数据块能立即发送，提高实时性
        , &optval // 指向存放选项值的缓冲区的指针，这里存储了是否启用该选项的值
        , sizeof optval); // sizeof optval: optval 缓冲区的大小（以字节为单位）

}



void Socket::setReuseAddr(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_
        , SOL_SOCKET // 操作套接字层
        , SO_REUSEADDR // 端口可以立即被复用
        , &optval
        , sizeof optval);
}

void Socket::setReusePort(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_,
        SOL_SOCKET,       // 操作套接字层
        // socket 封装了来源ip，不用担心同端口造成数据混乱
        SO_REUSEPORT,     // 允许同一端口被多个套接字绑定（多进程/多线程服务器）
        &optval,          // 指向存放选项值的缓冲区的指针
        sizeof optval);   // optval 缓冲区的大小（以字节为单位）
}

void Socket::setKeepAlive(bool on)
{
    int optval = on ? 1 : 0;
    ::setsockopt(sockfd_,
        SOL_SOCKET,       // 操作套接字层
        SO_KEEPALIVE,     // 启用TCP keepalive机制，检测死连接
        &optval,          // 指向存放选项值的缓冲区的指针
        sizeof optval);   // optval 缓冲区的大小（以字节为单位）
}