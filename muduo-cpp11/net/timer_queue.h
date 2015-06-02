// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_CPP11_NET_TIMER_QUEUE_H_
#define MUDUO_CPP11_NET_TIMER_QUEUE_H_

#include <set>
#include <utility>
#include <vector>

#include "muduo-cpp11/base/macros.h"
#include "muduo-cpp11/base/timestamp.h"
#include "muduo-cpp11/net/callbacks.h"
#include "muduo-cpp11/net/channel.h"

namespace muduo_cpp11 {
namespace net {

class EventLoop;
class Timer;
class TimerId;

///
/// A best efforts timer queue.
/// No guarantee that the callback will be on time.
///
class TimerQueue {
 public:
  TimerQueue(EventLoop* loop);
  ~TimerQueue();

  ///
  /// Schedules the callback to be run at given time,
  /// repeats if @c interval > 0.0.
  ///
  /// Must be thread safe. Usually be called from other threads.
  TimerId AddTimer(const TimerCallback& cb,
                   Timestamp when,
                   double interval);

  void Cancel(TimerId timerId);

#if defined(__MACH__) || defined(__ANDROID_API__)
  int GetTimeout() const;
  void ProcessTimers();
#endif

 private:

  // FIXME: use unique_ptr<Timer> instead of raw pointers.
  typedef std::pair<Timestamp, Timer*> Entry;
  typedef std::set<Entry> TimerList;
  typedef std::pair<Timer*, int64_t> ActiveTimer;
  typedef std::set<ActiveTimer> ActiveTimerSet;

  void AddTimerInLoop(Timer* timer);
  void CancelInLoop(TimerId timerId);

#if !defined(__MACH__) && !defined(__ANDROID_API__)
  // called when timerfd alarms
  void HandleRead();
#endif

  // move out all expired timers
  std::vector<Entry> GetExpired(Timestamp now);
  void Reset(const std::vector<Entry>& expired, Timestamp now);

  bool Insert(Timer* timer);

  EventLoop* loop_;
#if !defined(__MACH__) && !defined(__ANDROID_API__)
  const int timerfd_;
  Channel timerfd_channel_;
#endif

  // Timer list sorted by expiration
  TimerList timers_;

  // for cancel()
  ActiveTimerSet active_timers_;
  bool calling_expired_timers_; /* atomic */
  ActiveTimerSet canceling_timers_;
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_TIMER_QUEUE_H_
