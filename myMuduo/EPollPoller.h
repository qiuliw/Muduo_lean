#pragma once

#include "Channel.h"
#include "Poller.h"
#include "Timestamp.h"

#include <vector>
#include <sys/epoll.h>

/*
    epoll的使用
    epoll_create
    epoll_ctl add/mod/del
    epoll_wait
*/

// 事件分发器，相当于 epoll的封装,专门用于监听注册其上的channel_fd的IO事件，通知 Ractor 反应器去处理
class EPollPoller : public Poller
{
public:
    EPollPoller(EventLoop *loop);
    ~EPollPoller() override;

    // 重写基类Poller的抽象方法
    Timestamp poll(int timeout,ChannelList *activeChannels) override;
    void updateChannel(Channel* channel) override;
    void removeChannel(Channel* channel) override;

private:
    static const int kInitEventListSize = 16;

    // 填写活跃的连接
    void fillActiveChannels(int numEvents,ChannelList *activeChannels);
    // 更新channel通道
    void update(int operation,Channel* channel);

    using EventList = std::vector<epoll_event>;

    int epollfd_;
    EventList events_;
};