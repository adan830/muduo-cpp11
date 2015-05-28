// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_CPP11_NET_EVENT_LOOP_H_
#define MUDUO_CPP11_NET_EVENT_LOOP_H_

#include <atomic>
#include <memory>
#include <mutex>
#include <functional>
#include <vector>

#include <boost/any.hpp>

#include "muduo-cpp11/base/macros.h"
#include "muduo-cpp11/base/timestamp.h"
#include "muduo-cpp11/net/callbacks.h"
#include "muduo-cpp11/net/timer_id.h"

namespace muduo_cpp11 {
namespace net {

class Channel;
class Poller;
class TimerQueue;

///
/// Reactor, at most one per thread (IO thread).
///
/// This is an interface class, so don't expose too much details.
class EventLoop {
 public:
  typedef std::function<void()> Functor;

  EventLoop();
  ~EventLoop();

  ///
  /// Loops forever.
  ///
  /// Must be called in the same thread as creation of the object.
  ///
  void Loop();

  /// Quits loop.
  ///
  /// This is not 100% thread safe, if you call through a raw pointer,
  /// better to call through shared_ptr<EventLoop> for 100% safety.
  void Quit();

  ///
  /// Time when poll returns, usually means data arrival.
  ///
  Timestamp poll_return_time() const {
    return poll_return_time_;
  }

  /// Iteration count.
  int64_t iteration() const {
    return iteration_;
  }

  /// Runs callback immediately in the loop thread.
  ///
  /// It wakes up the loop, and run the cb.
  /// If in the same loop thread, cb is run within the function.
  ///
  /// Safe to call from other threads.
  void RunInLoop(const Functor& cb);

  /// Queues callback in the loop thread.
  ///
  /// Runs after finish pooling.
  ///
  /// Safe to call from other threads.
  void QueueInLoop(const Functor& cb);

  // timers

  ///
  /// Runs callback at 'time'.
  /// Safe to call from other threads.
  ///
  TimerId RunAt(const Timestamp& time, const TimerCallback& cb);

  ///
  /// Runs callback after @c delay seconds.
  /// Safe to call from other threads.
  ///
  TimerId RunAfter(double delay, const TimerCallback& cb);

  ///
  /// Runs callback every @c interval seconds.
  /// Safe to call from other threads.
  ///
  TimerId RunEvery(double interval, const TimerCallback& cb);

  ///
  /// Cancels the timer.
  /// Safe to call from other threads.
  ///
  void Cancel(TimerId timerId);

  // internal usage
  void Wakeup();
  void UpdateChannel(Channel* channel);
  void RemoveChannel(Channel* channel);
  bool HasChannel(Channel* channel);

  // pid_t thread_id() const { return thread_id_; }

  void AssertInLoopThread() {
    if (!IsInLoopThread()) {
      AbortNotInLoopThread();
    }
  }

  bool IsInLoopThread() const;
  // bool calling_pending_functors() const { return calling_pending_functors_; }

  bool EventHandling() const {
    return event_handling_;
  }

  // context
  void set_context(const boost::any& context) {
    context_ = context;
  }

  const boost::any& context() const {
    return context_;
  }

  boost::any* mutable_context() {
    return &context_;
  }

  static EventLoop* GetEventLoopOfCurrentThread();

 private:
  void AbortNotInLoopThread();
  void HandleRead();  // waked up
  void DoPendingFunctors();

  void PrintActiveChannels() const;  // DEBUG

 private:
  typedef std::vector<Channel*> ChannelList;

  std::atomic<bool> looping_;
  std::atomic<bool> quit_;
  std::atomic<bool> event_handling_;
  std::atomic<bool> calling_pending_functors_;

  int64_t iteration_;
  const pid_t thread_id_;

  Timestamp poll_return_time_;
  std::unique_ptr<Poller> poller_;
  std::unique_ptr<TimerQueue> timer_queue_;
  int wakeup_fd_;

  // unlike in TimerQueue, which is an internal class,
  // we don't expose Channel to client.
  std::unique_ptr<Channel> wakeup_channel_;

  boost::any context_;

  // scratch variables
  ChannelList active_channels_;
  Channel* current_active_channel_;

  std::mutex mutex_;
  std::vector<Functor> pending_functors_;  // @GuardedBy mutex_

  DISABLE_COPY_AND_ASSIGN(EventLoop);
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_EVENT_LOOP_H_
