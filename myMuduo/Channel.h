#pragma once

#include "Timestamp.h"
#include "noncopyable.h"

#include <algorithm>
#include <functional>
#include <memory>

class EventLoop;

/*
    Channal理解为通道，封装了sockfd及其感兴趣的event,如EPOLLIN,EPOLLOUT, 对应事件模型的分发器，负责具体的某个fd事件处理
    还绑定了poller返回的具体事件
*/
class Channel : noncopyable {
public:
    using EventCallback = std::function<void()>;
    using ReadEventCallback = std::function<void(Timestamp)>;

    Channel(EventLoop *eventLoop, int fd);
    ~Channel();

    // fd得到poller通知以后，处理事件的，调用相应的方法
    void handleEvent(Timestamp receiveTime);

    // 设置回调函数对象
    void setReadCallback(ReadEventCallback cb) {
        readCallback_ = std::move(cb);
    }
    void setWriteCallback(EventCallback cb) {
        writeCallback_ = std::move(cb);
    }
    void setCloseCallback(EventCallback cb) {
        closeCallback_ = std::move(cb);
    }
    void setErrorCallback(EventCallback cb) {
        errorCallback_ = std::move(cb);
    }

    // 防止当channel被手动remove掉，channal还在执行回调操作
    void tie(const std::shared_ptr<void> &);

    int fd() const {
        return fd_;
    }
    int events() const {
        return events_;
    }
    void set_revents(int revt) {
        revents_ = revt;
    }

    // 设置fd相应的事件状态
    // 通过 loop_->updateChannel(this) 来更新状态同步到poller的channelMap中
    void enableReading() {
        events_ |= kReadEvent;
        update();
    }
    void disableReading() {
        events_ &= ~kReadEvent;
        update();
    }
    void enableWriting() {
        events_ |= kWriteEvent;
        update();
    }
    void disableWriting() {
        events_ &= kWriteEvent;
        update();
    }
    void disableAll() {
        events_ = kNoneEvent;
        update();
    }

    // 返回fd当前的事件状态
    bool isNoneEvent() const {
        return events_ == kNoneEvent;
    }
    bool isWriting() const {
        return events_ & kWriteEvent;
    }
    bool isReading() const {
        return events_ & kReadEvent;
    }

    int index() {
        return index_;
    }
    void set_index(int idx) {
        index_ = idx;
    }

    // one loop per thread
    EventLoop *ownerLoop() {
        return loop_;
    }

    void remove();

private:
    void update();
    void handleEventWithGuard(Timestamp receiveTime);

    static const int kNoneEvent;
    static const int kReadEvent;
    static const int kWriteEvent;

    EventLoop *loop_; // 事件循环
    const int fd_;    // fd,Poller监听的对象
    int events_;      // 注册fd感兴趣的事件
    int revents_;     // poller返回具体发生的事件
    int index_;

    std::weak_ptr<void> tie_;
    bool tied_;

    // 因为channal通道里面能够获知fd最终发生的具体事件revents.
    // 所以他负责具体事件的回调操作
    ReadEventCallback readCallback_;
    EventCallback writeCallback_;
    EventCallback closeCallback_;
    EventCallback errorCallback_;
};