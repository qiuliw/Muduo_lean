#pragma once

#include "EventLoop.h"
#include "noncopyable.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class EventLoop;
class EventLoopThread;

//  线程池，负责轮询将新用户连接channel分配给subloop
class EventLoopThreadPool : noncopyable {

public:
  using ThreadInitCallback = std::function<void(EventLoop *)>;

  // baseLoop_ 用户提供的Loop
  EventLoopThreadPool(EventLoop *baseLoop_, const std::string &nameArg);
  ~EventLoopThreadPool();

  void setThreadNum(int numThreads) { numThreads_ = numThreads; }

  void start(const ThreadInitCallback &cb = ThreadInitCallback());

  // 如果是多线程中，baseLoop会默认以轮询方式分配chnnel给subloop
  EventLoop *getNextLoop();

  std::vector<EventLoop *> getAllLoops();

  bool started() const { return started_; }
  const std::string name() const { return name_; }

private:
  // 默认单线程模型
  EventLoop *baseLoop_; // EventLoop loop
  std::string name_;
  bool started_;
  int numThreads_;
  int next_;
  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop *> loops_;
};
