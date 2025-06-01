#include "EventLoopThread.h"
#include "EventLoop.h"
#include "Thread.h"

#include <condition_variable>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <thread>


// 将 EventLoop线程启动过程 回调 封装

// 形参默认值声明和初始化只其一定义一次，不能都定义默认值。
// 没有默认构造的成员，必须显式在初始化列表中构造,初始化列表调用成员的构造函数构造成员
EventLoopThread::EventLoopThread(const ThreadInitCallback &cb,
        const std::string &name)
    : loop_(nullptr)
    , exiting_(false)
    , thread_(std::bind(&EventLoopThread::threadFunc,this),name)
    , mutex_()
    , cond_()
    , callback_(cb)
{
    
}

EventLoopThread::~EventLoopThread()
{
    exiting_ = true;
    if(loop_ != nullptr){
        loop_->quit();
        thread_.join(); // 阻塞回收子线程
    }
}


EventLoop* EventLoopThread::startLoop()
{
    // one loop per thread
    // thread_.start() ==> new thread ==> threadFunc() ==>  EventLoop loop
    thread_.start(); // 开启一个新线程执行 threadFunc

    EventLoop *loop = nullptr;
    {
        std::unique_lock<std::mutex> lock(mutex_);
        while (loop_ == nullptr) { // 等待新创建的线程返回loop
            cond_.wait(lock);
        }
        loop = loop_;
    }

    return loop;
}



// 下面这个方法是在新线程中执行的
void EventLoopThread::threadFunc()
{
    EventLoop loop; // 创建一个独立的eventloop，和上面线程是一一对应的 one loop per thread

    // std::function 重载了 operator bool() 来判断是否绑定了有效的可调用对象
    if(callback_){
        callback_(&loop); 
    }

    {
        std::unique_lock<std::mutex> lock(mutex_);
        loop_ = &loop; // one loop per thread ; Eventloopthread --> thread --> loop
        cond_.notify_one();
    }

    loop.loop(); // EventLoop loop ==> poller.poll 
    // 如果执行到这，代表loop退出
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = nullptr;
}

