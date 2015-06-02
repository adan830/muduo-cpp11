// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Updated by Yifan Fan (yifan.fan.1983@gmail.com)

#include <stdlib.h>

#include "muduo-cpp11/net/poller.h"
#include "muduo-cpp11/net/poller/poll_poller.h"
#include "muduo-cpp11/net/poller/epoll_poller.h"

namespace muduo_cpp11 {
namespace net {

Poller* Poller::NewDefaultPoller(EventLoop* loop) {
#if defined(__MACH__) || defined(__ANDROID_API__)
  return new PollPoller(loop);
#else
  if (::getenv("MUDUO_CPP11_USE_POLL")) {
    return new PollPoller(loop);
  } else {
    return new EPollPoller(loop);
  }
#endif
}

}  // namespace net
}  // namespace muduo_cpp11
