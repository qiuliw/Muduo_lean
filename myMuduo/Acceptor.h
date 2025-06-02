#pragma once

#include "Channel.h"
#include "noncopyable.h"
#include "Socket.h"
#include <functional>

class EventLoop;
class InetAddress;

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

    // Acceptor用的就是用户定义的哪个baseLoop,也称作mainLoop
    EventLoop *loop_;
    Socket acceptSocket_;
    Channel acceptChannel_;
    NewConnectionCallback newConnectionCallback_;
    bool listenning_;
};