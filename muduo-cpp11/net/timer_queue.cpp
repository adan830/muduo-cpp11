// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)

#ifndef __STDC_LIMIT_MACROS
  #define __STDC_LIMIT_MACROS
#endif

#include "muduo-cpp11/net/timer_queue.h"

#include <assert.h>
#include <sys/timerfd.h>

#include <algorithm>
#include <functional>
#include <utility>
#include <vector>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/net/event_loop.h"
#include "muduo-cpp11/net/timer.h"
#include "muduo-cpp11/net/timer_id.h"

namespace muduo_cpp11 {
namespace net {

namespace detail {

int CreateTimerfd() {
  int timerfd = ::timerfd_create(CLOCK_MONOTONIC,
                                 TFD_NONBLOCK | TFD_CLOEXEC);
  if (timerfd < 0) {
    LOG(FATAL) << "Failed in timerfd_create";
  }
  return timerfd;
}

struct timespec HowMuchTimeFromNow(Timestamp when) {
  int64_t microseconds = when.microseconds_since_epoch()
                         - Timestamp::Now().microseconds_since_epoch();
  if (microseconds < 100) {
    microseconds = 100;
  }

  struct timespec ts;
  ts.tv_sec = static_cast<time_t>(
      microseconds / Timestamp::kMicroSecondsPerSecond);
  ts.tv_nsec = static_cast<int64_t>(
      (microseconds % Timestamp::kMicroSecondsPerSecond) * 1000);
  return ts;
}

void ReadTimerfd(int timerfd, Timestamp now) {
  uint64_t howmany;
  ssize_t n = ::read(timerfd, &howmany, sizeof howmany);
  VLOG(1) << "TimerQueue::HandleRead() " << howmany << " at " << now.ToString();
  if (n != sizeof howmany) {
    LOG(ERROR) << "TimerQueue::HandleRead() reads " << n
               << " bytes instead of 8";
  }
}

void ResetTimerfd(int timerfd, Timestamp expiration) {
  // wake up loop by timerfd_settime()
  struct itimerspec new_value;
  struct itimerspec old_value;
  bzero(&new_value, sizeof new_value);
  bzero(&old_value, sizeof old_value);
  new_value.it_value = HowMuchTimeFromNow(expiration);
  int ret = ::timerfd_settime(timerfd, 0, &new_value, &old_value);
  if (ret) {
    LOG(ERROR) << "timerfd_settime()";
  }
}

}  // namespace detail

TimerQueue::TimerQueue(EventLoop* loop)
    : loop_(loop),
      timerfd_(detail::CreateTimerfd()),
      timerfd_channel_(loop, timerfd_),
      timers_(),
      calling_expired_timers_(false) {
  timerfd_channel_.set_read_callback(
      std::bind(&TimerQueue::HandleRead, this));
  // we are always reading the timerfd, we disarm it with timerfd_settime.
  timerfd_channel_.EnableReading();
}

TimerQueue::~TimerQueue() {
  timerfd_channel_.DisableAll();
  timerfd_channel_.Remove();
  ::close(timerfd_);
  // do not remove channel, since we're in EventLoop::dtor();
  for (TimerList::iterator it = timers_.begin();
      it != timers_.end(); ++it) {
    delete it->second;
  }
}

TimerId TimerQueue::AddTimer(const TimerCallback& cb,
                             Timestamp when,
                             double interval) {
  Timer* timer = new Timer(cb, when, interval);
  loop_->RunInLoop(
      std::bind(&TimerQueue::AddTimerInLoop, this, timer));
  return TimerId(timer, timer->sequence());
}

void TimerQueue::Cancel(TimerId timerId) {
  loop_->RunInLoop(
      std::bind(&TimerQueue::CancelInLoop, this, timerId));
}

void TimerQueue::AddTimerInLoop(Timer* timer) {
  loop_->AssertInLoopThread();
  bool earliest_changed = Insert(timer);
  if (earliest_changed) {
    detail::ResetTimerfd(timerfd_, timer->expiration());
  }
}

void TimerQueue::CancelInLoop(TimerId timerId) {
  loop_->AssertInLoopThread();
  assert(timers_.size() == active_timers_.size());
  ActiveTimer timer(timerId.timer_, timerId.sequence_);
  ActiveTimerSet::iterator it = active_timers_.find(timer);
  if (it != active_timers_.end()) {
    size_t n = timers_.erase(Entry(it->first->expiration(), it->first));
    assert(n == 1); (void)n;
    delete it->first;  // FIXME: no delete please
    active_timers_.erase(it);
  } else if (calling_expired_timers_) {
    canceling_timers_.insert(timer);
  }
  assert(timers_.size() == active_timers_.size());
}

void TimerQueue::HandleRead() {
  loop_->AssertInLoopThread();
  Timestamp now(Timestamp::Now());
  detail::ReadTimerfd(timerfd_, now);

  std::vector<Entry> expired = GetExpired(now);

  calling_expired_timers_ = true;
  canceling_timers_.clear();
  // safe to callback outside critical section
  for (std::vector<Entry>::iterator it = expired.begin();
      it != expired.end(); ++it) {
    it->second->Run();
  }
  calling_expired_timers_ = false;

  Reset(expired, now);
}

std::vector<TimerQueue::Entry> TimerQueue::GetExpired(Timestamp now) {
  assert(timers_.size() == active_timers_.size());
  std::vector<Entry> expired;
  Entry sentry(now, reinterpret_cast<Timer*>(UINTPTR_MAX));
  TimerList::iterator end = timers_.lower_bound(sentry);
  assert(end == timers_.end() || now < end->first);
  std::copy(timers_.begin(), end, back_inserter(expired));
  timers_.erase(timers_.begin(), end);

  for (std::vector<Entry>::iterator it = expired.begin();
      it != expired.end(); ++it) {
    ActiveTimer timer(it->second, it->second->sequence());
    size_t n = active_timers_.erase(timer);
    assert(n == 1); (void)n;
  }

  assert(timers_.size() == active_timers_.size());
  return expired;
}

void TimerQueue::Reset(const std::vector<Entry>& expired, Timestamp now) {
  Timestamp next_expire;

  for (std::vector<Entry>::const_iterator it = expired.begin();
      it != expired.end(); ++it) {
    ActiveTimer timer(it->second, it->second->sequence());
    if (it->second->repeat()
        && canceling_timers_.find(timer) == canceling_timers_.end()) {
      it->second->Restart(now);
      Insert(it->second);
    } else {
      // FIXME move to a free list
      delete it->second;  // FIXME: no delete please
    }
  }

  if (!timers_.empty()) {
    next_expire = timers_.begin()->second->expiration();
  }

  if (next_expire.Valid()) {
    detail::ResetTimerfd(timerfd_, next_expire);
  }
}

bool TimerQueue::Insert(Timer* timer) {
  loop_->AssertInLoopThread();
  assert(timers_.size() == active_timers_.size());
  bool earliest_changed = false;
  Timestamp when = timer->expiration();
  TimerList::iterator it = timers_.begin();
  if (it == timers_.end() || when < it->first) {
    earliest_changed = true;
  }
  {
    std::pair<TimerList::iterator, bool> result
      = timers_.insert(Entry(when, timer));
    assert(result.second); (void)result;
  }
  {
    std::pair<ActiveTimerSet::iterator, bool> result
      = active_timers_.insert(ActiveTimer(timer, timer->sequence()));
    assert(result.second); (void)result;
  }

  assert(timers_.size() == active_timers_.size());
  return earliest_changed;
}

}  // namespace net
}  // namespace muduo_cpp11
