// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Updated by Yifan Fan (yifan.fan.1983@gmail.com)

#include "muduo-cpp11/net/poller/poll_poller.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/base/type_conversion.h"
#include "muduo-cpp11/net/channel.h"

namespace muduo_cpp11 {
namespace net {

PollPoller::PollPoller(EventLoop* loop)
    : Poller(loop) {
}

PollPoller::~PollPoller() {
}

Timestamp PollPoller::Poll(int timeout_ms, ChannelList* active_channels) {
  // XXX pollfds_ shouldn't change
  int num_events = ::poll(pollfds_.data(), pollfds_.size(), timeout_ms);
  int saved_errno = errno;
  Timestamp now(Timestamp::Now());
  if (num_events > 0) {
#if !defined(__MACH__) && !defined(__ANDROID_API__)
    if (VLOG_IS_ON(1)) {
      LOG(INFO) << num_events << " events happended";
    }
#endif
    FillActiveChannels(num_events, active_channels);
  } else if (num_events == 0) {
#if !defined(__MACH__) && !defined(__ANDROID_API__)
    if (VLOG_IS_ON(1)) {
      LOG(INFO) << " nothing happended";
    }
#endif
  } else {
    if (saved_errno != EINTR) {
      errno = saved_errno;
#if !defined(__MACH__) && !defined(__ANDROID_API__)
      LOG(ERROR) << "PollPoller::poll()";
#else
      LogError("PollPoller::poll()");
#endif
    }
  }
  return now;
}

void PollPoller::FillActiveChannels(int num_events,
                                    ChannelList* active_channels) const {
  for (PollFdList::const_iterator pfd = pollfds_.begin();
       pfd != pollfds_.end() && num_events > 0;
       ++pfd) {
    if (pfd->revents > 0) {
      --num_events;
      ChannelMap::const_iterator ch = channels_.find(pfd->fd);
      assert(ch != channels_.end());
      Channel* channel = ch->second;
      assert(channel->fd() == pfd->fd);
      channel->set_revents(pfd->revents);
      // pfd->revents = 0;
      active_channels->push_back(channel);
    }
  }
}

void PollPoller::UpdateChannel(Channel* channel) {
  Poller::AssertInLoopThread();

#if !defined(__MACH__) && !defined(__ANDROID_API__)
  if (VLOG_IS_ON(1)) {
    LOG(INFO) << "fd = " << channel->fd() << " events = " << channel->events();
  }
#endif

  if (channel->index() < 0) {
    // a new one, add to pollfds_
    assert(channels_.find(channel->fd()) == channels_.end());
    struct pollfd pfd;
    pfd.fd = channel->fd();
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    pollfds_.push_back(pfd);
    int idx = static_cast<int>(pollfds_.size()) - 1;
    channel->set_index(idx);
    channels_[pfd.fd] = channel;
  } else {
    // update existing one
    assert(channels_.find(channel->fd()) != channels_.end());
    assert(channels_[channel->fd()] == channel);
    int idx = channel->index();
    assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
    struct pollfd& pfd = pollfds_[idx];
    assert(pfd.fd == channel->fd() || pfd.fd == -channel->fd()-1);
    pfd.events = static_cast<short>(channel->events());
    pfd.revents = 0;
    if (channel->IsNoneEvent()) {
      // ignore this pollfd
      pfd.fd = -channel->fd() - 1;
    }
  }
}

void PollPoller::RemoveChannel(Channel* channel) {
  Poller::AssertInLoopThread();

#if !defined(__MACH__) && !defined(__ANDROID_API__)
  if (VLOG_IS_ON(1)) {
    LOG(INFO) << "fd = " << channel->fd();
  }
#endif

  assert(channels_.find(channel->fd()) != channels_.end());
  assert(channels_[channel->fd()] == channel);
  assert(channel->IsNoneEvent());
  int idx = channel->index();
  assert(0 <= idx && idx < static_cast<int>(pollfds_.size()));
  const struct pollfd& pfd = pollfds_[idx]; (void)pfd;
  assert(pfd.fd == -channel->fd() - 1 && pfd.events == channel->events());
  size_t n = channels_.erase(channel->fd());
  assert(n == 1); (void)n;
  if (implicit_cast<size_t>(idx) == pollfds_.size() - 1) {
    pollfds_.pop_back();
  } else {
    int channel_at_end = pollfds_.back().fd;
    iter_swap(pollfds_.begin() + idx, pollfds_.end() - 1);
    if (channel_at_end < 0) {
      channel_at_end = -channel_at_end - 1;
    }
    channels_[channel_at_end]->set_index(idx);
    pollfds_.pop_back();
  }
}

}  // namespace net
}  // namespace muduo_cpp11
