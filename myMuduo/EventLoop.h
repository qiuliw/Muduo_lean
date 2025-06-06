#pragma once

#include "noncopyable.h"
#include "Timestamp.h"
#include "CurrentThread.h"

#include <functional>
#include <vector>
#include <atomic>
#include <unistd.h>
#include <memory>
#include <mutex>
#include <thread>

class Channel;
class Poller;

// at most one per thread
// 事件循环类 主要包含了两个大模块 Channal Poller(epoll的抽象)

// 扮演Ractor反应器（事件反应器），从事件分发器中接受IO事件，创建新线程去执行channel事件回调
// 其同时也会创建一个Poller对象(事件分发器)，Poller对象负责监听Channel的IO事件
class EventLoop : noncopyable
{
public:
    using Functor = std::function<void()>;

    EventLoop();
    ~EventLoop();

    // 开启事件循环
    void loop();
    // 退出事件循环
    void quit();

    Timestamp pollReturnTime() const {
        return pollReturnTime_;
    }

    // 在当前loop对象绑定的线程中执行cb
    void runInLoop(Functor cb);
    // 把cb放入队列中，唤醒loop所在的线程执行cb，当前函数调用对象不在自身线程，唤醒自身线程去执行
    // 不在创建实例的线程内被调用，唤醒创建实例的线程
    void queueInLoop(Functor cb);

    // 唤醒loop所在的线程
    void wakeup(); // mainReactor唤醒subReactor

    // Eventloop的方法 ==> looper的方法
    void updateChannel(Channel *channel);
    void removeChannel(Channel *channel);
    void hasChannel(Channel *channel);

    // 判断 Eventloop对象是否在自己的线程里面
    bool isInLoopThread() const {
        // threadId_ 线程创建时的id
        return threadId_ == CurentThreead::tid();
    }

private:

    void handleRead(); // wakeup的回调
    void doPendingFunctors(); // 执行回调


    using ChannelList = std::vector<Channel*>;

    std::atomic_bool looping_; // 原子操作，通过CAS实现
    std::atomic_bool quit_; // 标志退出loop循环
    
    // 保证被创建eventloop的线程调用
    const pid_t threadId_; // 记录当前loop所在的线程id
    
    Timestamp pollReturnTime_; // poller返回发生事件的channels的时间点
    std::unique_ptr<Poller> poller_;

    // eventfd()
    // 主要作用，当mainLoop获取一个新用户的channel,通过轮询算法选择一个subloop
    // 通过该成员唤醒subloop处理channel
    // socketpair，由于有没有主动唤醒和采用网络机制，效率要低
    int wakeupFd_; 
    std::unique_ptr<Channel> wakeupChannel_;

    ChannelList activeChannels_;
    Channel *currentActiveChannel_;

    std::atomic_bool callingPendingFunctors_; // 标识当前loop是否有需要执行的回调操作 ；工作线程和IO线程同步状态使用，需要为原子操作
    std::vector<Functor> pendingFunctors_; // 存储loop需要执行的所有的回调操作
    std::mutex mutex_; // 互斥锁，用来保护上面vector容器的线程安全操作

};