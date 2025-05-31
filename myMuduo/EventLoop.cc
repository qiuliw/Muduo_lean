#include "EventLoop.h"
#include "Channel.h"
#include "CurrentThread.h"
#include "Logger.h"
#include "Poller.h"

#include <cerrno>
#include <cstdint>
#include <memory>
#include <mutex>
#include <sys/eventfd.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>

// 防止一个线程创建多个EventLoop

__thread EventLoop *t_loopInthisThread = nullptr;

// 定义默认的Poller IO复用接口的超时时间
const int kPollTimeMs = 10000;

// 创建wakeupfd，用来唤醒subReactor处理新来的channel
int createEventfd(){
    
    int evtfd = ::eventfd(0, EFD_NONBLOCK | EFD_CLOEXEC);// 不阻塞，不被继承
    if(evtfd < 0){
        LOG_FATAL("eventfd error:%d\n",errno);
    }

    return evtfd;
}

EventLoop::EventLoop()
    : looping_(false)
    , quit_(false)
    , callingPendingFunctors_(false)
    , threadId_(CurentThreead::tid())
    , poller_(Poller::newDefaultPoller(this))//创建poller
    , wakeupFd_(createEventfd()) // 唤醒子loop
    , wakeupChannel_(new Channel(this,wakeupFd_)) 
    , currentActiveChannel_(nullptr)
{
    LOG_DEBUG("EventLoop created %p in thread %d",this,threadId_);
    
    // 防止一个线程多个EventLoop
    if(t_loopInthisThread){
        LOG_FATAL("Another EventLoop %p exist",this);
    }else{
        t_loopInthisThread = this;
    }

    // 设置wakeup的事件类型以及发生事件后的回调
    wakeupChannel_->setReadCallback(std::bind(&EventLoop::handleRead,this));
    // 每一个eventloop都将监听wakeupChannel的EPOLLIN的读事件
    wakeupChannel_->enableReading();
}

EventLoop::~EventLoop(){
    wakeupChannel_->disableAll();
    wakeupChannel_->remove();
    ::close(wakeupFd_);
    t_loopInthisThread = nullptr;
}

void EventLoop::handleRead(){
    uint64_t one = 1;
    ssize_t n = read(wakeupFd_,&one,sizeof one);
    if(n != sizeof one){
        LOG_ERROR("EventLoop::handleRead() reads %d",n); 
    }
}

// 开启事件循环
void EventLoop::loop(){
    looping_ = true;
    quit_ = false;

    LOG_INFO("EventLoop %p start looping \n",this);

    while (!quit_) {
        activeChannels_.clear();
        // 监听两类fd 一种clientfd，一种wakeupfd
        pollReturnTime_ = poller_->poll(kPollTimeMs, &activeChannels_);
        
        
        for(Channel *channel : activeChannels_){
            // Poller监听哪些channel发生事件了，然后上报给Eventloop
            // 通知channel处理相应的事件
            channel->handleEvent(pollReturnTime_);
        }
        // 执行当前EventLoop事件循环需要处理的回调操作

        /*
            IO线程 mainLoop accept fd channel subloop
            mainLoop 事先注册一个回调cb （需要subloop来执行）
            wakeup subloop后，执行下面的方法，执行之前mainloop注册的回调
        */
        doPendingFunctors();
    }

    LOG_INFO("EventLoop %p stop looping. \n",this);
    looping_ = false;
}


/*
    mainloop

            no ======= 生产者-消费者的线程安全的队列

    subloop1 subloop2 subloop3
*/


// 退出事件循环
void EventLoop::quit(){
    quit_ = true;

    // 如果在本线程中，调用其他线程的quit
    // 马上唤醒其他线程退出
    if(!isInLoopThread()){
        wakeup();
    }
}

// 执行cb
void EventLoop::runInLoop(Functor cb){
    // loop.runInLoop 在当前loop线程中，执行cb
    if(isInLoopThread()){
        cb();
    }else{ // 在非当前loop线程中执行cb，就需要唤醒loop所在线程，执行cb
        queueInLoop(cb);
    }
}

// 把cb放入队列中，唤醒loop所在的线程，执行cb
void EventLoop::queueInLoop(Functor cb){
    {
        std::unique_lock<std::mutex> lokc(mutex_);
        // 用于在容器的尾部直接构造一个元素，右值会直接调用移动构造
        pendingFunctors_.emplace_back(cb);
    }

    // #1 obj1{obj2->loop->queueInloop}在obj1线程中调用obj2 loop obj1唤醒obj2去执行回调 
    // #2 本线程在dopendingfunctor()执行回调中，其他线程调用queueInLoop()为本线程添加了其他回调任务，
    //      则本线程需要在下次循环中唤醒去执行，为epoll添加事件避免阻塞
    if(!isInLoopThread() || callingPendingFunctors_){
        // callingPendingFunctors_ 逻辑待解释
        wakeup(); // 唤醒loop所在线程
    }
}

// 唤醒wakeup所属其他loop对象的线程
/*
    currentloop{
        otherloop.wakeup()
    }
    唤醒otherloop
*/
// 向wakeupfd写一个数据，唤醒她 ；wakeupChannel 就发生读事件，当前loop线程就会被唤醒
void EventLoop::wakeup(){
    uint64_t one = 1; // 值什么都可以
    ssize_t n = write(wakeupFd_,&one,sizeof one);
    if(n != sizeof one){ // 子线程唤醒失败
        LOG_ERROR("EventLoop::wakeup() write %lu bytes instead of 8 \n",n); //instead of 代替；而不是；取代
    }
}

// channel ==> Eventloop的方法 ==> looper的方法
// channel 通过父组件 Eventloop 间接 更新 looper
void EventLoop::updateChannel(Channel *channel){
    poller_->updateChannel(channel);
}
void EventLoop::removeChannel(Channel *channel){
    poller_->removeChannel(channel);
}
void EventLoop::hasChannel(Channel *channel){
    poller_->hasChannel(channel);
}

// 执行回调
void EventLoop::doPendingFunctors(){
    std::vector<Functor> functors;
    callingPendingFunctors_ = true; // 表明当前线程执行回调中，有其他回调添加队列后需要向wakeupfd_通知再次唤醒，下轮epoll执行
    
    {  // 将回调序列交换到局部，避免影响主线程插入回调效率（主线程插入回调需要用锁，而工作执行删除回调也需要锁）
        std::unique_lock<std::mutex> lock(mutex_);
        functors.swap(pendingFunctors_);
    }

    for(const Functor &functor: functors){ 
        functor(); // 执行当前loop需要执行的回调函数
    }
    callingPendingFunctors_ = false;
}