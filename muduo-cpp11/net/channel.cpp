// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)

#include "muduo-cpp11/net/channel.h"

#include <assert.h>
#include <poll.h>

#include <sstream>
#include <string>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/net/event_loop.h"

using std::string;

namespace muduo_cpp11 {
namespace net {

const int Channel::kNoneEvent = 0;
const int Channel::kReadEvent = POLLIN | POLLPRI;
const int Channel::kWriteEvent = POLLOUT;

Channel::Channel(EventLoop* loop, int fd)
    : loop_(loop),
      fd_(fd),
      events_(0),
      revents_(0),
      index_(-1),
      log_hup_(true),
      tied_(false),
      event_handling_(false),
      added_to_loop_(false) {
}

Channel::~Channel() {
  assert(!event_handling_);
  assert(!added_to_loop_);
  if (loop_->IsInLoopThread()) {
    assert(!loop_->HasChannel(this));
  }
}

void Channel::Tie(const std::shared_ptr<void>& obj) {
  tie_ = obj;
  tied_ = true;
}

void Channel::Update() {
  added_to_loop_ = true;
  loop_->UpdateChannel(this);
}

void Channel::Remove() {
  assert(IsNoneEvent());
  added_to_loop_ = false;
  loop_->RemoveChannel(this);
}

void Channel::HandleEvent(Timestamp receive_time) {
  std::shared_ptr<void> guard;
  if (tied_) {
    guard = tie_.lock();
    if (guard) {
      HandleEventWithGuard(receive_time);
    }
  } else {
    HandleEventWithGuard(receive_time);
  }
}

void Channel::HandleEventWithGuard(Timestamp receive_time) {
  event_handling_ = true;
  VLOG(1) << REventsToString();

  if ((revents_ & POLLHUP) && !(revents_ & POLLIN)) {
    if (log_hup_) {
      LOG(WARNING) << "Channel::handle_event() POLLHUP";
    }

    if (close_callback_) {
      close_callback_();
    }
  }

  if (revents_ & POLLNVAL) {
    LOG(WARNING) << "Channel::handle_event() POLLNVAL";
  }

  if (revents_ & (POLLERR | POLLNVAL)) {
    if (error_callback_) error_callback_();
  }

  if (revents_ & (POLLIN | POLLPRI | POLLRDHUP)) {
    if (read_callback_) read_callback_(receive_time);
  }

  if (revents_ & POLLOUT) {
    if (write_callback_) write_callback_();
  }

  event_handling_ = false;
}

string Channel::REventsToString() const {
  return EventsToString(fd_, revents_);
}

string Channel::EventsToString() const {
  return EventsToString(fd_, events_);
}

string Channel::EventsToString(int fd, int ev) {
  std::ostringstream oss;
  oss << fd << ": ";
  if (ev & POLLIN) oss << "IN ";
  if (ev & POLLPRI) oss << "PRI ";
  if (ev & POLLOUT) oss << "OUT ";
  if (ev & POLLHUP) oss << "HUP ";
  if (ev & POLLRDHUP) oss << "RDHUP ";
  if (ev & POLLERR) oss << "ERR ";
  if (ev & POLLNVAL) oss << "NVAL ";
  return oss.str().c_str();
}

}  // namespace net
}  // namespace muduo_cpp11
