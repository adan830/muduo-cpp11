// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)

#include "muduo-cpp11/net/poller.h"
#include "muduo-cpp11/net/channel.h"

namespace muduo_cpp11 {
namespace net {

Poller::Poller(EventLoop* loop)
    : owner_loop_(loop) {
}

Poller::~Poller() {
}

bool Poller::HasChannel(Channel* channel) const {
  AssertInLoopThread();
  ChannelMap::const_iterator it = channels_.find(channel->fd());
  return it != channels_.end() && it->second == channel;
}

}  // namespace net
}  // namespace muduo_cpp11
