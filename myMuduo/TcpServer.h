#pragma once

/*
    用户使用modue编写服务器程序
*/
#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

#include "Acceptor.h"
#include "Callbacks.h"
#include "EventLoop.h"
#include "EventLoopThreadPool.h"
#include "InetAddress.h"
#include "Socket.h"
#include "noncopyable.h"

// 对外的服务器编程使用的类

// 封装 acceptor - listen - new connection 新连接建立与封装
// TcpServer类是一个服务器端的类，负责监听新连接、处理连接和消息等。
class TcpServer : noncopyable {
public:
    using ThreadInitCallback = std::function<void(EventLoop *)>;

    enum Option {
        kNoReusePort,
        kReusePort,
    };

    TcpServer(EventLoop *loop, 
        const InetAddress &listenAddr, 
        const std::string &nameArg,
        Option option = kNoReusePort);
    ~TcpServer();

    void setThreadInitCallback(const ThreadInitCallback &cb) {
        threadInitCallback_ = cb;
    }
    void setConnectionCallback(const ConnectionCallback &cb) {
        connectionCallback_ = cb;
    }
    void setMessageCallback(const MessageCallback &cb) {
        messageCallback_ = cb;
    }
    void setWriteCompleteCallback(const WriteCompleteCallback &cb) {
        writeCompleteCallback_ = cb;
    }

    // 设置底层subloop的个数
    void setThreadNum(int numThreads);

    // 开启服务器监听
    void start();

private:
    // peer 对端。
    void newConnection(int sockfd, const InetAddress &peerAddr);
    void removeConnection(const TcpConnectionPtr &conn);
    void removeConnectionInLoop(const TcpConnectionPtr &conn);

    using ConnectionMap = std::unordered_map<std::string, TcpConnectionPtr>;

    EventLoop *loop_; // baseLoop  用户定义的loop
    const std::string name_;
    std::unique_ptr<Acceptor> acceptor_;                // 运行在mainLoop，任务就是监听新连接事件
    std::shared_ptr<EventLoopThreadPool> threadPool_; // one loop per thread

    ConnectionCallback connectionCallback_;       // 有新连接时的回调
    MessageCallback messageCallback_;             // 有读写消息时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成后的回调

    ThreadInitCallback threadInitCallback_; // loop线程初始化的回调
    std::atomic_int started_;

    int nextConnId_;
    ConnectionMap connections_; // 保存所有连接
    std::string ipPort_; // 服务器监听的端口号
};
