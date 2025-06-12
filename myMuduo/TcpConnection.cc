#include "TcpConnection.h"
#include "EventLoop.h"
#include "Logger.h"
#include "Socket.h"
#include "Channel.h"
#include "Callbacks.h"

#include <cstddef>
#include <cstdlib>
#include <errno.h>
#include <sys/socket.h>
#include <sys/types.h>


static EventLoop* checkLoopNotNull(EventLoop *loop)
{
    if(loop == nullptr)
    {
        LOG_FATAL("mainLoop is null!");
    }
    return loop;
}

TcpConnection::TcpConnection(EventLoop *loop,
                const std::string &nameArg,
                int sockfd,
                const InetAddress &localAddr,
                const InetAddress &peerAddr)
    : loop_(checkLoopNotNull(loop))
    , name_(nameArg)
    , state_(kConnecting)
    , reading_(true)
    , socket_(new Socket(sockfd))
    , channel_(new Channel(loop, sockfd))
    , localAddr_(localAddr)
    , peerAddr_(peerAddr)
    , highWaterMark_(64*1024*1024) // 64M
{
    // 下面给channel设置相应的回调函数，poller给channel通知感兴趣的事件发生了，channel会回调
    channel_->setReadCallback(
        std::bind(&TcpConnection::handleRead, this, std::placeholders::_1)
    );
    channel_->setWriteCallback(
        std::bind(&TcpConnection::handleWrite, this)
    );
    channel_->setCloseCallback(
        std::bind(&TcpConnection::handleClose, this)
    );
    channel_->setErrorCallback(
        std::bind(&TcpConnection::handleError, this)
    );
    
    // ctor Constructor
    LOG_INFO("TcpConnection::ctor[%s] at fd=%d\n", name_.c_str(), sockfd);
    
    socket_->setKeepAlive(true); // tcp保活
}



TcpConnection::~TcpConnection()
{
    // dtor Destructor
    LOG_INFO("TcpConnection::dtor[%s] at fd=%d state=%d\n", name_.c_str(), channel_->fd(), static_cast<int>(state_));
}

void TcpConnection::handleRead(Timestamp receiveTime)
{
    int savedErrno = 0;
    // 网络 --> inBuffer_.writable bytes
    size_t n = inputBuffer_.readFd(channel_->fd(), &savedErrno);
    if(n > 0){
        // 已建立连接的用户，有可读事件发生了，调用用户传入的回调操作onMessage
        messageCallback_(shared_from_this(), &inputBuffer_,receiveTime);
    }
    // 非阻塞模式，无数据可读会返回-1, 0表示连接关闭
    // 但是无数据可读事件不应该被poller呈递，出现-1视作发生了错误
    else if(n == 0){
        handleClose();
    }
    else{
        errno = savedErrno;
        LOG_ERROR("TcpConnection::handleRead");
        handleError();
    }
}


// 自动将缓存中的数据全部发送
void TcpConnection::handleWrite()
{
    //  channel_有可写事件
    if(channel_->isWriting())
    {
        int savedErrno = 0;
        // outBuffer_.readable bytes --> 网络
        ssize_t n = outputBuffer_.writeFd(channel_->fd(), &savedErrno);
        if(n>0){
            outputBuffer_.retrieve(n); // 读取消费了数据，更新 readerIndex_
            if(outputBuffer_.readableBytes() == 0){ // 数据消费光
                channel_->disableWriting(); // 不再监听可写事件
                if(writeCompleteCallback_) // 写完 回调
                {
                    // 唤醒loop_对应的thread线程，执行回调
                    loop_->queueInLoop(
                        std::bind(writeCompleteCallback_, shared_from_this())
                    );
                }
                // 数据返回完成，如果状态是正在断开连接，则关闭连接；
                // 由用户主动调用shutdown,等待缓冲区写完数据后关闭连接
                if(state_ == kDisconnecting){
                    shutdownInLoop();
                }
            }
            else{
                LOG_ERROR("TcpConnection::handleWrite");
            }
        }
    }else{ // 对写事件不感兴趣
        LOG_ERROR("TcpConnection fd=%d is down, no more writing\n", channel_->fd());
    }
}

void TcpConnection::handleClose()
{
    LOG_INFO("fd=%d, state=%d\n",channel_->fd(), static_cast<int>(state_));
    setState(kDisconnected);
    channel_->disableAll();

    TcpConnectionPtr connPtr(shared_from_this()); 
    // 连接关闭的回调 
    // 调用一个未绑定的 std::function 是安全的，它不会执行任何操作，相当于“空操作”。
    connectionCallback_(connPtr); // 传递智能指针，保证对象存活
    closeCallback_(connPtr);
}

void TcpConnection::handleError()
{
    int optval;
    socklen_t optlen = sizeof optval;
    int err = 0;
    if(::getsockopt(channel_->fd(), SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0)
    {
        err = errno;
    }else{
        err = optval;
    }
    LOG_ERROR("TcpConnection::handleError name:%s - SO_ERROR:%d\n", name_.c_str(), err);
}

//  发送数据，应用写的快，而内核发送数据慢，需要把待发送数据放入缓冲区，而且判断水位回调
// 直接尝试发送数据，如果发送没有发送完成，需要把未发送完的数据buf保存起来，由handleWrite负责完成发送
void TcpConnection::sendInLoop(const void  *data, size_t len)
{
    ssize_t nwrote = 0;
    size_t remaining = len;
    bool faultError = false;

    // 之前调用过该connection shutdown，不能再发送了
    if(state_ == kDisconnected){
        LOG_ERROR("disconnected, give up writing");
        return;
    }

    // 如果有其他channel注册了写监听，或者缓冲区有数据，则按照流水线规则要将数据放入缓冲区等待
    // 判断写事件是否注册到Poller注册，并检查输出缓冲区是否为空
    // 表示channel_第一次开始写数据，而且缓冲区没有待发送数据
    // channel_->isWriting() 检查是否注册了 监听可读事件
    // 未写完调用TcpConnection::handleWrite
    if(!channel_->isWriting() && outputBuffer_.readableBytes() == 0)
    {
        // 虽然没监听可写事件，试着直接发送
        // 采用 “乐观发送” 策略，在没有待发送数据时，直接尝试发送，减少系统调用的开销。
        nwrote = ::write(channel_->fd(), data, len);
        if(nwrote >= 0){
            remaining = len - nwrote;
            if(remaining == 0 && writeCompleteCallback_){
                // 既然在这里数据全部发送完成，就不用再给channel设置epollout事件监听
                loop_->queueInLoop(
                    std::bind(writeCompleteCallback_, shared_from_this())
                );
            }
        }
        else// nwrote < 0
        { 
            nwrote = 0;
            // ，if(errno != EWOULDBLOCK) 的判断用于区分 “暂时不可用” 和 “真正的错误”
            if(errno != EWOULDBLOCK) // EWOULDBLOCK 非阻塞模式中操作被阻塞无法完成返回错误，暂时不可用
            {
                LOG_ERROR("TcpConnection::sendInLoop");
                //  EPIPE 连接已关闭，ECONNRESET tcp rst 包连接重置，非正常关闭
                if(errno == EPIPE || errno == ECONNRESET)
                {
                    faultError = true;
                }
            }
        }
    }


    // 说明缓冲区有数据未发送，剩余的数据需要保存到缓冲区当中，然后给channel注册可写事件
    // 注册epollout事件，poller发现tcp的发送缓冲区有剩余空间，会通知相应的socket-channel,调用writeCallvack_
    // 也就是调用  TcpConnection::handleWrite，直到把数据全部发送完成
    // 高水位：发送缓冲区数据量 > highWaterMark
    if(!faultError && remaining > 0){
        // 目前发送缓冲区剩余的待发送数据
        size_t oldLen = outputBuffer_.readableBytes();
        // 是否触发高水位回调
        if(oldLen + remaining >= highWaterMark_ 
            && oldLen < highWaterMark_
            && highWaterMarkCallback_)
        {
            loop_->queueInLoop(
                std::bind(highWaterMarkCallback_, shared_from_this(),oldLen + remaining)
            );
        }
        outputBuffer_.append((char*)data + nwrote, remaining);
        if(!channel_->isWriting()){
            // 不断监听可写事件
            channel_->enableWriting(); // 记得注册channel的写事件，否则poller不会给channel通知epollout
        }
    }   

}

// 连接建立：一个新连接创建的时候
void TcpConnection::connectEstablished()
{
    setState(kConnected);
    channel_->tie(shared_from_this()); // 绑定TcpConnection对象,防止TcpConnection对象被销毁
    channel_->enableReading(); // 注册channel的读事件

    // 新连接建立，执行回调
    connectionCallback_(shared_from_this());
}

// 连接销毁
void TcpConnection::connectDestroyed()
{
    if(state_ == kConnected){
        setState(kDisconnecting);
        channel_->disableAll(); // 把channel的所有感兴趣的事件，从poller中移除
        connectionCallback_(shared_from_this());
    }
    channel_->remove(); // 把channel从poller中删除
}

void TcpConnection::shutdown(){
    if(state_ == kConnected){
        setState(kDisconnecting); // 等待缓冲区数据发送完成，再关闭连接
        loop_->runInLoop(
            std::bind(&TcpConnection::shutdownInLoop, this) // 尝试关闭
        );
    }
}

// 如果没有写事件，直接关闭写端，触发连接关闭回调
// 如果有写事件，则检测到kDisconnected状态，则写完关闭连接
void TcpConnection::shutdownInLoop()
{
    if(!channel_->isWriting()){
        socket_->shutdownWrite(); // 关闭写端，触发半连接，handleClose()==>closeCallback_ 和 connectionCallback_；
    }
    
}

void TcpConnection::send(const std::string &buf)
{
    if(state_ == kConnected){
        sendInLoop(buf.c_str(),buf.size());
    }
    else{
        loop_->runInLoop(std::bind(
            &TcpConnection::sendInLoop, 
            this, 
            buf.c_str(), 
            buf.size())
        );
    }
}
