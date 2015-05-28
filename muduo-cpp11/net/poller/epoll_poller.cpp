// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Updated by Yifan Fan (yifan.fan.1983@gmail.com)

#include "muduo-cpp11/net/poller/epoll_poller.h"

#include <assert.h>
#include <errno.h>
#include <poll.h>
#include <sys/epoll.h>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/net/channel.h"

namespace muduo_cpp11 {
namespace net {

// On Linux, the constants of poll(2) and epoll(4)
// are expected to be the same.
static_assert(EPOLLIN == POLLIN, "EPOLLIN == POLLIN");
static_assert(EPOLLPRI == POLLPRI, "EPOLLPRI == POLLPRI");
static_assert(EPOLLOUT == POLLOUT, "EPOLLOUT == POLLOUT");
static_assert(EPOLLRDHUP == POLLRDHUP, "EPOLLRDHUP == POLLRDHUP");
static_assert(EPOLLERR == POLLERR, "EPOLLERR == POLLERR");
static_assert(EPOLLHUP == POLLHUP, "EPOLLHUP == POLLHUP");

namespace {
const int kNew = -1;
const int kAdded = 1;
const int kDeleted = 2;
}

EPollPoller::EPollPoller(EventLoop* loop)
    : Poller(loop),
      epollfd_(::epoll_create1(EPOLL_CLOEXEC)),
      events_(kInitEventListSize) {
  if (epollfd_ < 0) {
    LOG(FATAL) << "EPollPoller::EPollPoller";
  }
}

EPollPoller::~EPollPoller() {
  ::close(epollfd_);
}

Timestamp EPollPoller::Poll(int timeout_ms, ChannelList* active_channels) {
  int num_events = ::epoll_wait(epollfd_,
                                events_.data(),
                                static_cast<int>(events_.size()),
                                timeout_ms);
  int saved_errno = errno;
  Timestamp now(Timestamp::Now());
  if (num_events > 0) {
    if (VLOG_IS_ON(1)) {
      LOG(INFO) << num_events << " events happended";
    }
    FillActiveChannels(num_events, active_channels);
    if (implicit_cast<size_t>(num_events) == events_.size()) {
      events_.resize(events_.size() * 2);
    }
  } else if (num_events == 0) {
    if (VLOG_IS_ON(1)) {
      LOG(INFO) << " nothing happended";
    }
  } else {
    // error happens, log uncommon ones
    if (saved_errno != EINTR) {
      errno = saved_errno;
      LOG(ERROR) << "EPollPoller::poll()";
    }
  }
  return now;
}

void EPollPoller::FillActiveChannels(int num_events,
                                     ChannelList* active_channels) const {
  assert(implicit_cast<size_t>(num_events) <= events_.size());
  for (int i = 0; i < num_events; ++i) {
    Channel* channel = static_cast<Channel*>(events_[i].data.ptr);
#ifndef NDEBUG
    int fd = channel->fd();
    ChannelMap::const_iterator it = channels_.find(fd);
    assert(it != channels_.end());
    assert(it->second == channel);
#endif
    channel->set_revents(events_[i].events);
    active_channels->push_back(channel);
  }
}

void EPollPoller::UpdateChannel(Channel* channel) {
  Poller::AssertInLoopThread();
  if (VLOG_IS_ON(1)) {
    LOG(INFO) << "fd = " << channel->fd() << " events = " << channel->events();
  }
  const int index = channel->index();
  if (index == kNew || index == kDeleted) {
    // a new one, add with EPOLL_CTL_ADD
    int fd = channel->fd();
    if (index == kNew) {
      assert(channels_.find(fd) == channels_.end());
      channels_[fd] = channel;
    } else {  // index == kDeleted
      assert(channels_.find(fd) != channels_.end());
      assert(channels_[fd] == channel);
    }
    channel->set_index(kAdded);
    Update(EPOLL_CTL_ADD, channel);
  } else {
    // update existing one with EPOLL_CTL_MOD/DEL
    int fd = channel->fd();
    (void)fd;
    assert(channels_.find(fd) != channels_.end());
    assert(channels_[fd] == channel);
    assert(index == kAdded);
    if (channel->IsNoneEvent()) {
      Update(EPOLL_CTL_DEL, channel);
      channel->set_index(kDeleted);
    } else {
      Update(EPOLL_CTL_MOD, channel);
    }
  }
}

void EPollPoller::RemoveChannel(Channel* channel) {
  Poller::AssertInLoopThread();
  int fd = channel->fd();
  if (VLOG_IS_ON(1)) {
    LOG(INFO) << "fd = " << fd;
  }
  assert(channels_.find(fd) != channels_.end());
  assert(channels_[fd] == channel);
  assert(channel->IsNoneEvent());
  int index = channel->index();
  assert(index == kAdded || index == kDeleted);
  size_t n = channels_.erase(fd);
  (void)n;
  assert(n == 1);

  if (index == kAdded) {
    Update(EPOLL_CTL_DEL, channel);
  }
  channel->set_index(kNew);
}

void EPollPoller::Update(int operation, Channel* channel) {
  struct epoll_event event;
  bzero(&event, sizeof event);
  event.events = channel->events();
  event.data.ptr = channel;
  int fd = channel->fd();
  if (::epoll_ctl(epollfd_, operation, fd, &event) < 0) {
    if (operation == EPOLL_CTL_DEL) {
      LOG(ERROR) << "epoll_ctl op=" << operation << " fd=" << fd;
    } else {
      LOG(FATAL) << "epoll_ctl op=" << operation << " fd=" << fd;
    }
  }
}

}  // namespace net
}  // namespace muduo_cpp11
