#include "TcpServer.h"
#include "Acceptor.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Logger.h"
#include "TcpConnection.h"
#include <cstddef>
#include <cstdio>
#include <strings.h>
#include "Buffer.h"


// #x 替换为 x 的字面量字符串 "x"  例如 x是ptr，则#x会被替换为 "ptr"

EventLoop* checkNotNull(EventLoop *loop) {
    if (loop == nullptr) {
        LOG_FATAL("%s:%s:%d mainLoop is null!\n",
                  __FILE__, __FUNCTION__, __LINE__);
    }
    return loop;
}

TcpServer::TcpServer(EventLoop *loop,
                    const InetAddress &listenAddr,
                    const std::string &nameArg,
                    Option option)
    : loop_(checkNotNull(loop))
    , ipPort_(listenAddr.toIpPort())
    , name_(nameArg)
    , acceptor_(new Acceptor(loop, listenAddr, option))
    , threadPool_(new EventLoopThreadPool(loop, name_))
    , connectionCallback_()
    , messageCallback_()
    , nextConnId_(1)
    , started_(0)
{
    // 当有新用户连接时，会执行TcpServer::newConnection方法回调
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, 
            std::placeholders::_1, std::placeholders::_2));
}

TcpServer::~TcpServer()
{
    for(auto &item : connections_){
        // TcpConnectionPtr 创建一个局部指针对象，离开这个作用域自动销毁对象
        TcpConnectionPtr conn(item.second);
        item.second.reset(); // 解除这个指针的引用，仅保留刚创建的

        // 销毁连接
        conn->getLoop()->runInLoop([conn](){
            conn->connectDestroyed();
        });
    }
}

// 设置底层subloop个数
void TcpServer::setThreadNum(int numThreads)
{
    threadPool_->setThreadNum(numThreads);
}

// 开启服务器监听
void TcpServer::start()
{
    if(started_++ == 0){ // 防止一个Tcp对象被start多次
        threadPool_->start(threadInitCallback_); // 启动底层loop线程池
        loop_->runInLoop(std::bind(&Acceptor::listen,acceptor_.get()));
    }
}

// 有一个新的客户端连接，acceptor会执行这个回调操作
void TcpServer::newConnection(int sockfd,const InetAddress &peerAddr)
{
    // 轮询算法，选择一个subLoop来管理channel
    EventLoop *ioLoop = threadPool_->getNextLoop();
    char buf[64] = {0};

    snprintf(buf, sizeof buf, "-%s#%d",ipPort_.c_str(), nextConnId_);
    ++nextConnId_;

    std::string connName = name_ + buf;

    LOG_INFO("TcpServer::newConnection [%s] - new connection [%s] from %s"
        , name_.c_str()
        , connName.c_str()
        , peerAddr.toIpPort().c_str()
    );

    // 通过sockfd获取其绑定的本机的ip和port
    sockaddr_in local;
    ::bzero(&local, sizeof local);
    socklen_t addrlen = sizeof local;
    if(::getsockname(sockfd,(sockaddr*)&local, &addrlen)<0){
        LOG_ERROR("sockets::getLocalAddr");
    }
    InetAddress localAddr(local);

    // 根据连接成功的socket文件描述符创建TcpConnection对象
    TcpConnectionPtr conn(new TcpConnection(ioLoop,
                                    connName, 
                                    sockfd,
                                    localAddr,
                                    peerAddr));
    
    connections_[connName] = conn;
    // 下面回调都是用户设置给TcpServer=>TcpConnection=>Channel=>Poller=>notify
    conn->setConnectionCallback(connectionCallback_);
    conn->setMessageCallback(messageCallback_);
    conn->setWriteCompleteCallback(writeCompleteCallback_);

    // 类成员函数作为回调需要传递对象
    // 设置了如何关闭连接的回调
    conn->setCloseCallback([this](const TcpConnectionPtr& conn) {
        this->removeConnection(conn);
    });

    // 直接调用TcpConnection::connectEstablished
    // 将conn绑定到channel上,防止失效
    ioLoop->runInLoop([conn]() {
        conn->connectEstablished();
    });
}

void TcpServer::removeConnection(const TcpConnectionPtr &conn)
{
    loop_->runInLoop([this, conn]() {
        removeConnectionInLoop(conn);
    });
}

void TcpServer::removeConnectionInLoop(const TcpConnectionPtr &conn)
{ 
    LOG_INFO("TcpServer::removeConnectionInLoop  [%s] - connection %s\n",
    name_.c_str(), conn->name().c_str());

    connections_.erase(conn->name()); // 从TcpServer的map中删除
    
    EventLoop *ioLoop = conn->getLoop();
    ioLoop->queueInLoop([conn]() {
        conn->connectDestroyed(); // channel->closeCallback --> tcpconn::handleClose
    });

}