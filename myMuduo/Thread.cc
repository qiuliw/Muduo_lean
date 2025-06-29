#include "Thread.h"

#include "CurrentThread.h"

#include <cstdio>
#include <memory>
#include <thread>
#include <unistd.h>
#include <semaphore.h>

std::atomic_int Thread::numCreated_(0);

Thread::Thread(ThreadFunc func,const std::string &name)
    : started_(false)
    , joined_(false)
    , tid_(0)
    , func_(std::move(func))
    , name_(name)
{
    setDefaultName();

}

Thread::~Thread(){
    // 创建线程且线程未被回收
    if(started_ && !joined_){
        thread_->detach(); // thread类提供的设置线程分离的方法
    }
}

void Thread::start() // 一个Thread对象，记录的就是一个新线程的详细信息
{
    started_ = true;
    sem_t sem;
    sem_init(&sem, false, 0); // 进程间不共享false
    // 开启线程
    thread_ = std::shared_ptr<std::thread>(new std::thread([&](){
        // 获取线程的pid
        tid_ = CurentThreead::tid();
        sem_post(&sem);
        func_(); // 开启一个新线程，专门执行该线程函数
    }));

    // 这里必须等待上面新创建的线程的tid值被赋值
    sem_wait(&sem);


}

void Thread::join(){
    joined_ = true;
    thread_->join();
}

void Thread::setDefaultName(){
    int num = ++numCreated_;
    if(name_.empty()){
        char buf[32] = {0};
        snprintf(buf, sizeof buf, "Thread%d", num);  // 正确：限制写入长度
    }
}