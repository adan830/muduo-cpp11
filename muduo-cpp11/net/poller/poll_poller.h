// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Updated by Yifan Fan (yifan.fan@dian.fm)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_CPP11_NET_POLLER_POLL_POLLER_H_
#define MUDUO_CPP11_NET_POLLER_POLL_POLLER_H_

#include "muduo-cpp11/net/poller.h"

#include <vector>

struct pollfd;

namespace muduo_cpp11 {
namespace net {

///
/// IO Multiplexing with poll(2).
///
class PollPoller : public Poller {
 public:
  explicit PollPoller(EventLoop* loop);
  virtual ~PollPoller();

  virtual Timestamp Poll(int timeout_ms, ChannelList* active_channels);
  virtual void UpdateChannel(Channel* channel);
  virtual void RemoveChannel(Channel* channel);

 private:
  void FillActiveChannels(int num_events,
                          ChannelList* active_channels) const;

  typedef std::vector<struct pollfd> PollFdList;
  PollFdList pollfds_;
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_POLLER_POLL_POLLER_H_
