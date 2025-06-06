#include "TcpServer.h"
#include "Acceptor.h"
#include "EventLoop.h"
#include "InetAddress.h"
#include "Logger.h"


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
{
    // 当有新用户连接时，会执行TcpServer::newConnection方法回调
    acceptor_->setNewConnectionCallback(
        std::bind(&TcpServer::newConnection, this, 
            std::placeholders::_1, std::placeholders::_2));
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

void TcpServer::newConnection(int sockfd,const InetAddress &peerAddr)
{

}