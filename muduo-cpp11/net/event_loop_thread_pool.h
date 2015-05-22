// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_CPP11_NET_EVENT_LOOP_THREAD_POOL_H_
#define MUDUO_CPP11_NET_EVENT_LOOP_THREAD_POOL_H_

#include <atomic>
#include <memory>
#include <functional>
#include <vector>

#include "muduo-cpp11/base/macros.h"

namespace muduo_cpp11 {
namespace net {

class EventLoop;
class EventLoopThread;

class EventLoopThreadPool {
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  explicit EventLoopThreadPool(EventLoop* base_loop);
  ~EventLoopThreadPool();

  void set_thread_num(int num_threads);
  void Start(const ThreadInitCallback& cb = ThreadInitCallback());

  // valid after calling Start()
  // round-robin
  EventLoop* GetNextLoop();

  // with the same hash code, it will always return the same EventLoop
  EventLoop* GetLoopForHash(size_t hash_code);

  // valid after calling Start()
  std::vector<EventLoop*> GetAllLoops();

  bool started() const {
    return started_;
  }

 private:
  EventLoop* base_loop_;
  std::atomic<bool> started_;

  int num_threads_;
  int next_;

  std::vector<std::unique_ptr<EventLoopThread>> threads_;
  std::vector<EventLoop*> loops_;

  DISABLE_COPY_AND_ASSIGN(EventLoopThreadPool);
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_EVENT_LOOP_THREAD_POOL_H_
