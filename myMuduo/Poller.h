#pragma once

#include "Channel.h"
#include "Timestamp.h"
#include "noncopyable.h"

#include <unordered_map>
#include <vector>

class Channel;
class EventLoop;

// muduo库中多路事件分发器的核心IO复用模块
class Poller : noncopyable{
public:
    using ChannelList = std::vector<Channel*>;

    Poller(EventLoop *loop);
    virtual ~Poller() = default;

    // 给所有IO复用保留同一的接口
    virtual Timestamp poll(int timeout,ChannelList *activeChannels) = 0;
    virtual void updateChannel(Channel* channel) = 0;
    virtual void removeChannel(Channel* channel) = 0;

    // 判断参数channel是否在当前Poller当中
    bool hasChannel(Channel *channel) const;

    // 工厂类，返回具体实现的实例
    // EventLoop可以通过该接口获取默认的IO复用的具体实现
    // 静态工厂类，不需要类实例化，返回具体的Poller实现类对象
    static Poller* newDefaultPoller(EventLoop *loop);

protected:
    // map的key：sockfd,value: sockfd所属的通道类型
    using ChannelMap = std::unordered_map<int, Channel*>;
    ChannelMap channels_;

private:
    EventLoop* ownerLoop_; // 定义Poller所属的事件循环EventLoop

};