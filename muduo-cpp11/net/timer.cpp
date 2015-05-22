// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)

#include "muduo-cpp11/net/timer.h"

namespace muduo_cpp11 {
namespace net {

std::atomic<int64_t> Timer::s_num_created_(0);

void Timer::Restart(Timestamp now) {
  if (repeat_) {
    expiration_ = AddTime(now, interval_);
  } else {
    expiration_ = Timestamp::Invalid();
  }
}

}  // namespace net
}  // namespace muduo_cpp11
