// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_CPP11_NET_POLLER_H_
#define MUDUO_CPP11_NET_POLLER_H_

#include <map>
#include <vector>

#include "muduo-cpp11/base/macros.h"
#include "muduo-cpp11/base/timestamp.h"
#include "muduo-cpp11/net/event_loop.h"

namespace muduo_cpp11 {
namespace net {

class Channel;

///
/// Base class for IO Multiplexing
///
/// This class doesn't own the Channel objects.
class Poller {
 public:
  typedef std::vector<Channel*> ChannelList;

  Poller(EventLoop* loop);
  virtual ~Poller();

  /// Polls the I/O events.
  /// Must be called in the loop thread.
  virtual Timestamp Poll(int timeout_ms, ChannelList* active_channels) = 0;

  /// Changes the interested I/O events.
  /// Must be called in the loop thread.
  virtual void UpdateChannel(Channel* channel) = 0;

  /// Remove the channel, when it destructs.
  /// Must be called in the loop thread.
  virtual void RemoveChannel(Channel* channel) = 0;

  virtual bool HasChannel(Channel* channel) const;

  static Poller* NewDefaultPoller(EventLoop* loop);

  void AssertInLoopThread() const {
    owner_loop_->AssertInLoopThread();
  }

 protected:
  typedef std::map<int, Channel*> ChannelMap;
  ChannelMap channels_;

 private:
  EventLoop* owner_loop_;

  DISABLE_COPY_AND_ASSIGN(Poller);
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_POLLER_H_
