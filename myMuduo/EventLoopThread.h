#pragma once

#include "Thread.h"
#include "noncopyable.h"

#include <condition_variable>
#include <functional>
#include <mutex>
#include <string>

class EventLoop;


// per EventLoopThread ==> one Thread ==> one EventLoop
class EventLoopThread : noncopyable
{
public:
    // 线程池初始化后的回调类型
    using ThreadInitCallback = std::function<void(EventLoop*)>;

    EventLoopThread(const ThreadInitCallback &cb = ThreadInitCallback(),
        const std::string &nam = std::string());
    ~EventLoopThread();

    EventLoop* startLoop();
    
private:

    void threadFunc();

    EventLoop *loop_;
    bool exiting_; // 是否退出循环
    Thread thread_;
    std::mutex mutex_;
    std::condition_variable cond_; // 主动唤醒其他线程
    ThreadInitCallback callback_; // 线程池回调
};