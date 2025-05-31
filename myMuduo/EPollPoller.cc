#include "EPollPoller.h"
#include "Channel.h"
#include "Poller.h"
#include "Logger.h"
#include "Timestamp.h"

#include <cerrno>
#include <cstring>
#include <sys/epoll.h>
#include <errno.h>
#include <unistd.h>  // close

// channel未添加到poller中
const int kNew = -1; // channel的成员 index_ = -1
// channel 已添加到poller中
const int kAdded = 1;
// channel 从poller中删除
const int kDeleted = 2;

EPollPoller::EPollPoller(EventLoop *loop)
    :Poller(loop)
    // 进程被fork后fd不被继承
    , epollfd_(::epoll_create1(EPOLL_CLOEXEC))
    , events_(kInitEventListSize) // vector<epoll_events>
{
    if(epollfd_<0){
        LOG_FATAL("epoll_create error:%d \n",errno);
    }
}
// 析构可以为虚函数，会被基类指针自动调用子类虚析构函数再调用基类虚析构函数，保证子类资源正确被释放
EPollPoller::~EPollPoller(){
    ::close(epollfd_);
}

// 维护当前正在epoll中的channel_fd集合
// epoll_wait => activeChannels 返回发生事件的channel和发生事件的时间
Timestamp EPollPoller::poll(int timeoutMs,ChannelList *activeChannels){
    // 实际上用LOG_DEBUG输出日志更为合理
    LOG_INFO("func%s => fd total count:%d\n",__FUNCTION__,static_cast<int>(events_.size()));

    // events_.begin() => iterator ==> *iterator返回首元素，*被迭代器重载，不能忽略 &*，maxevents 一次性最多返回的事件数量
    // int numEvents = ::epoll_wait(epollfd_, &*events_.begin(),static_cast<int>(events_.size()),timeoutMs);
    // 如果使用 C++11 或更高版本，可以直接用 events_.data() 代替 &*events_.begin()：
    int numEvents = ::epoll_wait(epollfd_, events_.data(),static_cast<int>(events_.size()),timeoutMs);
    int saveErrno = errno;
    Timestamp now(Timestamp::now());

    if(numEvents >0){
        // DEBUG
        LOG_INFO("%d events happened \n",numEvents);
        fillActiveChannels(numEvents, activeChannels);
        if(numEvents == events_.size()){
            events_.resize(events_.size()*2); // 扩容
        }
    }else if (numEvents == 0 ) { // 无事件而返回，说明timeout超时
        LOG_DEBUG("%s timeout!\n",__FUNCTION__);
    }else{ // 因外部中断而返回
        if(saveErrno != EINTR){
            errno = saveErrno; // 保证输出的是epoll_wait返回时的errno 在日志系统中的errno
            LOG_ERROR("EPollPoller::poll() err!");
        }
    }
    return now;
}

// Poller 是基类，可以有不同是实现 epoll poll

// epoll_ctl
// 根据 channelList 确定具体的 epoll控制字
// channel感兴趣的事情改变，更新channels_映射epoll。将channel添加到fd，如果没有感兴趣的事情就del从epoll
// kDeleted也要再次被添加，因为updateChannel只有在事件改变时被调用，调用即表示kDeleted状态不再
void EPollPoller::updateChannel(Channel* channel){
    const int index = channel->index();
    LOG_INFO("func=%s => fd=%d events=%d index=%d \n",__FUNCTION__,channel->fd(),channel->events(),index);

    if(index == kNew || index == kDeleted)
    {
        int fd = channel->fd();
        if(index == kNew)
        {
            channels_[fd] = channel;   // 对于新事件，先添加到本地映射，已经有映射的不需要
        } 
        
        channel->set_index(kAdded);
        update(EPOLL_CTL_ADD,channel);
    }else{
        // update existing one with EPOLL_CTL_MOD/DEL
        int fd = channel->fd();
        if(channel->isNoneEvent()){
            update(EPOLL_CTL_DEL,channel);
            channel->set_index(kDeleted);
        }
        else{
            update(EPOLL_CTL_MOD, channel);
        }
    }

}

// 从poller中完全移除channel的逻辑
void EPollPoller::removeChannel(Channel* channel){

    int index = channel->index();

    int fd = channel->fd();
    channels_.erase(fd); // channels_ 中删除

    LOG_INFO("func=%s => fd=%d\n",__FUNCTION__,fd);

    if(index == kAdded){
        update(EPOLL_CTL_DEL,channel); // 从epoll中删除
    }
    channel->set_index(kNew);

}
// 填写活跃的连接
void EPollPoller::fillActiveChannels(int numEvents,ChannelList *activeChannels){
    for(int i=0;i < numEvents;++i){
        Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
        channel->set_revents(events_[i].events);
        // EPollLoop就拿到了它的poller给他返回的所有发生事件的channel列表
        activeChannels->push_back(channel);

    }
}

// 更新 epoll_ctl
void EPollPoller::update(int operation,Channel* channel){
    epoll_event event;
    // typedef union epoll_data
    // {
    //   void *ptr;
    //   int fd;
    //   uint32_t u32;
    //   uint64_t u64;
    // } epoll_data_t;

    // struct epoll_event
    // {
    //   uint32_t events;	/* Epoll events */
    //   epoll_data_t data;	/* User data variable */
    // } __EPOLL_PACKED;
    memset(&event, 0, sizeof event);
    event.events = channel->events(); // 绑定感兴趣的事件
    event.data.ptr= channel; // 绑定关联的channel
    int fd = channel->fd(); // 指定fd
    event.data.fd = fd;
    if(::epoll_ctl(epollfd_, operation,fd, &event) < 0){
        if(operation == EPOLL_CTL_DEL){
            LOG_ERROR("epoll_ctl del error:%d\n",errno);
        }else{
            LOG_FATAL("epoll_ctl del error:%d\n",errno);
        }
    }

}