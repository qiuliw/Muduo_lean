#include "Acceptor.h"
#include "Logger.h"
#include "InetAddress.h"

#include <cerrno>
#include <cstdio>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>



// static 关键字表示该函数具有内部链接（internal linkage）​，即它只能在当前源文件（.cpp 或 .c 文件）中使用，其他文件无法访问。
// 默认 extern
static int createNonBlocking()
{
    int sockfd = ::socket(AF_INET // IPV4
        ,SOCK_STREAM // TCP
            | SOCK_NONBLOCK  // 非阻塞IO
            | SOCK_CLOEXEC  // 使套接字在调用 exec() 系列函数（如 fork() + exec() 启动新程序）时 ​自动关闭。
        ,IPPROTO_TCP // 指定使用 ​TCP 协议​（传输控制协议）
    );
    
    if(sockfd < 0){
        LOG_FATAL("%s:%s:%d listen socket create err:%d",__FILE__,__FUNCTION__,__LINE__,errno);
    }

    return sockfd;
}

Acceptor::Acceptor(EventLoop *loop,const InetAddress &listenAddr,bool reuseport)
    : loop_(loop)
    , acceptSocket_(createNonBlocking()) // socket_create
    , acceptChannel_(loop,acceptSocket_.fd())
    , listenning_(false)
{
    acceptSocket_.setReuseAddr(true);
    acceptSocket_.setReusePort(true);
    acceptSocket_.bindAddress(listenAddr); // bind
    // TcpServer::start() Acceptor.listen 有新用户连接，要执行一个回调
    // connfd ==> channel ==> subloop

    // baseLoop => accpetChannel_(listenfd)
    acceptChannel_.setReadCallback(std::bind(&Acceptor::handleRead,this));
}
Acceptor::~Acceptor(){
    acceptChannel_.disableAll();
    acceptChannel_.remove();
}


void Acceptor::listen(){
    listenning_ = true;
    acceptSocket_.listen(); // listen
    acceptChannel_.enableReading(); // acceptChannel => Poller
}

// listenfd有事件发生了，就是有新用户连接了
void Acceptor::handleRead(){
    InetAddress peerAddr;
    int connfd = acceptSocket_.accept(&peerAddr);
    if(connfd >=0){
        if(newConnectionCallback_){
            newConnectionCallback_(connfd,peerAddr); // 轮询找到subLoop,唤醒，分发当前的新客户端channel
        }else{
            ::close(connfd);
        }
    }else{
        LOG_ERROR("%s:%s:%d accept err:%d \n",__FILE__,__FUNCTION__,__LINE__,errno);
        if(errno == EMFILE){ // fd无空余
            LOG_ERROR("%s:%s:%d sockfd reached limit!\n",__FILE__,__FUNCTION__,__LINE__);
        }
    }
}