#pragma once

#include "Buffer.h"
#include "EventLoop.h"
#include "noncopyable.h"
#include "InetAddress.h"
#include "Callbacks.h"

#include <atomic>
#include <cstddef>
#include <memory>
#include <string>

class Channel;
class EventLoop;
class Socket;

/*
    TcpServer => Acceptor => 有一个新用户连接，通过accept函数拿到connfd
    => TcpConnection设置回调 => Channel => Poller => 触发回调 => 应用层处理数据 
*/

// TcpServer -->  TcpConnection --> Channel

// TcpConnction负责管理Channel之上的连接状态，负责发送和接收数据。
// 属于channel绑定的资源


// 用户设置的回调被封装成TcpConnection，tcpConnection再注册到channel上
// 例如 readCallback =>  TcpConnection.setReadCallback(readCallback); => handleRead{readCallback} ==> channel->{enabelRead{handleRead{readCallback}}}
// channel仅负责从poller中获取事件，并调用TcpConnection的回调函数处理事件。连接的数据在TcpConnection中处理。


// Acceptor和TcpConnect都是将逻辑细化注册到channel中，TcpConnection负责处理Tcp连接。
class TcpConnection : noncopyable
                        , public std::enable_shared_from_this<TcpConnection>
{
    
public:

    enum State {
        kConnecting, // 正在连接
        kConnected, // 已经连接
        kDisconnecting, //  正在断开连接
        kDisconnected // 断开连接
    };


    TcpConnection(EventLoop *loop,
                const std::string &nameArg,
                int sockfd,
                const InetAddress &localAddr,
                const InetAddress &peerAddr);
    ~TcpConnection();

    EventLoop *getLoop() const { return loop_; }
    const std::string& name() const {return name_;}
    const InetAddress& localAddr() const { return localAddr_; }
    const InetAddress& peerAddr() const {  return peerAddr_; }
    
    bool connected() const { return state_ == kConnected; }

    // 发送数据
    void send(const std::string &buf);
    // 关闭连接
    void shutdown();

    void setState(State state){
        state_ = state;
    }

    void setConnectionCallback(const ConnectionCallback &cb)
    { connectionCallback_ = cb; }

    void setMessageCallback(const MessageCallback &cb)
    { messageCallback_ = cb; }

    void setCloseCallback(const CloseCallback &cb)
    { closeCallback_ = cb; }

    void setWriteCompleteCallback(const WriteCompleteCallback &cb)
    { writeCompleteCallback_ = cb; }

    void setHighWaterMarkCallback(const HighWaterMarkCallback &cb, size_t mark)
    { highWaterMarkCallback_ = cb; highWaterMark_ = mark; }

    // 连接建立和销毁的回调
    void connectEstablished();
    void connectDestroyed();




private:


    // 事件处理函数
    void handleRead(Timestamp receiveTime);
    void handleWrite();
    void handleClose();
    void handleError();

    void sendInLoop(const void  *data, size_t len);

    void shutdownInLoop();

    EventLoop *loop_; 
    const std::string name_;
    std::atomic_int state_;
    bool  reading_;

    // 这里和Acceptor类似， Acceptor => mainLoop 
    // TcpConnection => subLoop
    std::unique_ptr<Socket> socket_;
    std::unique_ptr<Channel> channel_;

    const InetAddress localAddr_;
    const InetAddress peerAddr_;

    ConnectionCallback connectionCallback_; // 有新连接时的回调
    MessageCallback messageCallback_; // 有读写消息时的回调
    CloseCallback closeCallback_; // 连接关闭时的回调
    WriteCompleteCallback writeCompleteCallback_; // 消息发送完成以后的回调
    HighWaterMarkCallback  highWaterMarkCallback_; // 发送缓冲区高水位回调
    
    size_t highWaterMark_;

    // 两个缓冲区，读缓冲和写缓冲
    Buffer inputBuffer_;
    Buffer outputBuffer_;

};