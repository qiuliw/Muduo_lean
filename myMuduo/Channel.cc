#include "Channel.h"
#include "EventLoop.h"

#include <memory>
#include <sys/epoll.h>

const int Channel::kNoneEvent = 0;
// POLLPRI ​兼容性​：某些旧代码或特殊协议可能依赖 OOB(TCP带外数据) 数据。
const int Channel::kReadEvent = EPOLLIN | EPOLLPRI;
const int Channel::kWriteEvent = EPOLLOUT;

Channel::Channel(EventLoop *loop,int fd)
    : loop_(loop)
    , fd_(fd)
    , events_(0)
    , revents_(0)
    , index_(-1)
    , tied_(false)
{

}

Channel::~Channel()
{

}

// Channel 握有obj资源句柄，防止对象被释放
void Channel::tie(const std::shared_ptr<void>& obj){
    tie_ = obj;
    tied_ = true;
}

// 当改变channel所表示的events事件后，update负责在poller里面更改fd相应的事件epoll_ctl
// EventLoop => channelList事件资源管理  poller事件监听管理
void Channel::update(){
    // 通过channel所属的EventLoop，调用poller的相应方法，注册fd的events事件
    // add code...
    loop_->updateChannel(this);
}

// 在channel所属的Eventloop中，把当前channel删除掉
void Channel::remove(){
    // add code
    loop_->removeChannel(this);
}
// fd得到poller通知以后，处理事件的，调用相应的方法
void Channel::handleEvent(Timestamp receiveTime){
    if(tied_){
        std::shared_ptr<void> guard = tie_.lock();
        if(guard){
            handleEventWithGuard(receiveTime);
        }
    }else{
        handleEventWithGuard(receiveTime);
    }
}

// 根据poller通知的channel发生的具体事件，有channel负责调用具体的回调操作
void Channel::handleEventWithGuard(Timestamp receiveTime)
{
    if((revents_ & EPOLLHUP) && !(revents_ & EPOLLIN)){
        // 处理对端关闭且没有数据可读的情况
        if(closeCallback_){
            closeCallback_();
        }
    }
    if(revents_ & EPOLLERR){
        // 错误
        if(errorCallback_){
            errorCallback_();
        }
    }
    if(revents_ & (EPOLLIN | EPOLLPRI)){
        readCallback_(receiveTime);
    }

    if(revents_ & EPOLLOUT){
        writeCallback_();
    }
}