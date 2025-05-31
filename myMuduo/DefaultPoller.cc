#include "EPollPoller.h"
#include "Poller.h"

#include <cstdlib>
#include <stdlib.h>

// 静态工厂类，返回具体的子类实现，
// 单独定义，避免在顶层基类引用子类，污染顶层
Poller* Poller::newDefaultPoller(EventLoop* loop){
    if(::getenv("MUDUO_USE_POLL")){
        return nullptr; // 生成poll的实例
    }
    else{
        return new EPollPoller(loop); // 生成epoll的实例
    }
}