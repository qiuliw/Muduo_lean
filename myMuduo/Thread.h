#pragma once

#include "noncopyable.h"

#include <functional>
#include <thread>
#include <memory>
#include <unistd.h>
#include <string>
#include <atomic>

class Thread : noncopyable{
public:
    using ThreadFunc = std::function<void()>;

    explicit Thread(ThreadFunc func,const std::string &name = std::string());
    ~Thread();

    void start();
    void join();

    bool start() const { return started_;}
    pid_t tid() const { return tid_;}
    const std::string& name() const { return name_;}
    
    static int numCreated(){ return numCreated_; }
private:
    void setDefaultName();

    bool started_;
    bool joined_;
    // std::thread thread_; 线程对象创建时直接启动
    std::shared_ptr<std::thread> thread_; // 避免线程对象创建时启动，使用指针管理
    pid_t tid_;
    ThreadFunc func_;
    std::string name_;
    static std::atomic_int numCreated_;
};