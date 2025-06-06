#pragma once

#include "Channel.h"
#include "noncopyable.h"
#include "Socket.h"
#include <functional>

class EventLoop;
class InetAddress;

// 负责创建 listen socket 并且封装为 acceptorChannel
// 放入 poller 中监听 accept socket 的读事件
// 当有新的连接到来时，acceptorChannel 会调用 handleRead() 方法
// 在 handleRead() 方法中，acceptor 会调用 accept() 接受连接
// 并且通过回调函数 newConnectionCallback_ 接受用户从TcpServer中设置的回调函数 ewConnectionCallback_
// 其作为一个特殊的channel, 在mainLoop中注册
// 扮演新连接分发器的角色，负责监听新连接的到来，并通过负载均衡将新连接分发给subLoop去处理
class Acceptor : noncopyable{
public:
    using NewConnectionCallback = std::function<void(int sockfd,const InetAddress&)>;
    Acceptor(EventLoop *loop,const InetAddress &listenAddr,bool reuseport);
    ~Acceptor();

    void setNewConnectionCallback(const NewConnectionCallback &cb)
    {
        newConnectionCallback_ = std::move(cb);
    }

    bool listenning() const { return listenning_;}
    void listen();

private:

    void handleRead();

    // Acceptor用的就是用户定义的那个baseLoop,也称作mainLoop 
    EventLoop *loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};