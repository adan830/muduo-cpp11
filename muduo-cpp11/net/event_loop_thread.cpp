// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)

#include "muduo-cpp11/net/event_loop_thread.h"
#include "muduo-cpp11/net/event_loop.h"

namespace muduo_cpp11 {
namespace net {

EventLoopThread::EventLoopThread(const ThreadInitCallback& cb)
    : loop_(NULL),
      exiting_(false),
      init_callback_(cb) {
}

EventLoopThread::~EventLoopThread() {
  exiting_ = true;

  // not 100% race-free, eg. ThreadFunc could be running init_callback_.
  if (loop_ != NULL) {
    // still a tiny chance to call destructed object, if ThreadFunc
    // exits just now. But when EventLoopThread destructs, usually
    // programming is exiting anyway.
    loop_->Quit();
    thread_->join();
  }
}

EventLoop* EventLoopThread::StartLoop() {
  thread_.reset(new std::thread(std::bind(&EventLoopThread::ThreadFunc, this)));

  {
    std::unique_lock<std::mutex> lock(mutex_);
    while (loop_ == NULL) {
      cond_.wait(lock);
    }
  }

  return loop_;
}

void EventLoopThread::ThreadFunc() {
  EventLoop loop;

  if (init_callback_) {
    init_callback_(&loop);
  }

  {
    std::unique_lock<std::mutex> lock(mutex_);
    loop_ = &loop;
    cond_.notify_one();
  }

  loop.Loop();
  // assert(exiting_);
  loop_ = NULL;
}

}  // namespace net
}  // namespace muduo_cpp11
