// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Updated by Yifan Fan (yifan.fan@dian.fm)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_CPP11_NET_POLLER_EPOLL_POLLER_H_
#define MUDUO_CPP11_NET_POLLER_EPOLL_POLLER_H_

#include "muduo-cpp11/net/poller.h"

#include <vector>

struct epoll_event;

namespace muduo_cpp11 {
namespace net {

///
/// IO Multiplexing with epoll(4).
///
class EPollPoller : public Poller {
 public:
  explicit EPollPoller(EventLoop* loop);
  virtual ~EPollPoller();

  virtual Timestamp Poll(int timeout_ms, ChannelList* active_channels);
  virtual void UpdateChannel(Channel* channel);
  virtual void RemoveChannel(Channel* channel);

 private:
  static const int kInitEventListSize = 16;

  void FillActiveChannels(int num_events,
                          ChannelList* active_channels) const;
  void Update(int operation, Channel* channel);

  typedef std::vector<struct epoll_event> EventList;

  int epollfd_;
  EventList events_;
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_POLLER_EPOLL_POLLER_H_
