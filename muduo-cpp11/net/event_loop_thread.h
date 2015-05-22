// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_CPP11_NET_EVENT_LOOP_THREAD_H_
#define MUDUO_CPP11_NET_EVENT_LOOP_THREAD_H_

#include <atomic>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <thread>

#include "muduo-cpp11/base/macros.h"

namespace muduo_cpp11 {
namespace net {

class EventLoop;

class EventLoopThread {
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  EventLoopThread(const ThreadInitCallback& cb = ThreadInitCallback());
  ~EventLoopThread();

  EventLoop* StartLoop();

 private:
  void ThreadFunc();

 private:
  EventLoop* loop_;
  std::atomic<bool> exiting_;
  std::unique_ptr<std::thread> thread_;
  std::mutex mutex_;
  std::condition_variable cond_;
  ThreadInitCallback init_callback_;

  DISABLE_COPY_AND_ASSIGN(EventLoopThread);
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_EVENT_LOOP_THREAD_H_
