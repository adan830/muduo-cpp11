// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_CPP11_NET_TIMER_H_
#define MUDUO_CPP11_NET_TIMER_H_

#include <atomic>

#include "muduo-cpp11/base/macros.h"
#include "muduo-cpp11/base/timestamp.h"
#include "muduo-cpp11/net/callbacks.h"

namespace muduo_cpp11 {
namespace net {

///
/// Internal class for timer event.
///
class Timer {
 public:
  Timer(const TimerCallback& cb, Timestamp when, double interval)
      : callback_(cb),
        expiration_(when),
        interval_(interval),
        repeat_(interval > 0.0),
        sequence_(s_num_created_.fetch_add(1) + 1) {
  }

  void Run() const {
    callback_();
  }

  Timestamp expiration() const  { return expiration_; }
  bool repeat() const { return repeat_; }
  int64_t sequence() const { return sequence_; }

  void Restart(Timestamp now);

  static int64_t num_created() { return s_num_created_.load(); }

 private:
  const TimerCallback callback_;
  Timestamp expiration_;
  const double interval_;
  const bool repeat_;
  const int64_t sequence_;

  static std::atomic<int64_t> s_num_created_;

  DISABLE_COPY_AND_ASSIGN(Timer);
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_TIMER_H_
