// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)

#include "muduo-cpp11/net/event_loop_thread_pool.h"

#include <assert.h>
#include <functional>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/net/event_loop.h"
#include "muduo-cpp11/net/event_loop_thread.h"

namespace muduo_cpp11 {
namespace net {

EventLoopThreadPool::EventLoopThreadPool(EventLoop* base_loop)
    : base_loop_(base_loop),
      started_(false),
      num_threads_(0),
      next_(0) {
}

EventLoopThreadPool::~EventLoopThreadPool() {
  // Don't delete loop, it's stack variable
}

void EventLoopThreadPool::set_thread_num(int num_threads) {
  CHECK_GE(num_threads, 0) << "num_threads should >= 0";
  num_threads_ = num_threads;
}

void EventLoopThreadPool::Start(const ThreadInitCallback& cb) {
  assert(!started_);
  base_loop_->AssertInLoopThread();

  started_ = true;

  for (int i = 0; i < num_threads_; ++i) {
    std::unique_ptr<EventLoopThread> t(new EventLoopThread(cb));
    loops_.push_back(t->StartLoop());
    threads_.push_back(std::move(t));
  }

  if (num_threads_ == 0 && cb) {
    cb(base_loop_);
  }
}

EventLoop* EventLoopThreadPool::GetNextLoop() {
  base_loop_->AssertInLoopThread();
  assert(started_);
  EventLoop* loop = base_loop_;

  if (!loops_.empty()) {
    // round-robin
    loop = loops_[next_];
    ++next_;
    if (implicit_cast<size_t>(next_) >= loops_.size()) {
      next_ = 0;
    }
  }
  return loop;
}

EventLoop* EventLoopThreadPool::GetLoopForHash(size_t hash_code) {
  base_loop_->AssertInLoopThread();
  EventLoop* loop = base_loop_;
  if (!loops_.empty()) {
    loop = loops_[hash_code % loops_.size()];
  }
  return loop;
}

std::vector<EventLoop*> EventLoopThreadPool::GetAllLoops() {
  base_loop_->AssertInLoopThread();
  assert(started_);
  if (loops_.empty()) {
    return std::vector<EventLoop*>(1, base_loop_);
  } else {
    return loops_;
  }
}

}  // namespace net
}  // namespace muduo_cpp11
