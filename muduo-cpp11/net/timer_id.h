// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_CPP11_NET_TIMER_ID_H_
#define MUDUO_CPP11_NET_TIMER_ID_H_

namespace muduo_cpp11 {
namespace net {

class Timer;

///
/// An opaque identifier, for canceling Timer.
///
class TimerId {
 public:
  TimerId() : timer_(NULL), sequence_(0) {
  }

  TimerId(Timer* timer, int64_t seq) : timer_(timer), sequence_(seq) {
  }

  // default copy-ctor, dtor and assignment are okay

  friend class TimerQueue;

 private:
  Timer* timer_;
  int64_t sequence_;
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_TIMER_ID_H_
