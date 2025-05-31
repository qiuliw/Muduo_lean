#include "Poller.h"

Poller::Poller(EventLoop *loop)
    : ownerLoop_(loop)
{

}

bool Poller::hasChannel(Channel *channel) const{
    auto it = channels_.find(channel->fd());
    return it != channels_.end() && it->second == channel;
}

// #include "PollPoller.h"
// #include "EPollPoller.h"
// // 返回具体IO，但本类是顶层抽象类，不应该依赖底层。所以需要在其他源文件中引用底层实现 => DefaultPoller.cc
// Poller* Poller::newDefaultPoller(EventLoop *loop){

// }
