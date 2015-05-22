// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_CPP11_NET_CHANNEL_H_
#define MUDUO_CPP11_NET_CHANNEL_H_

#include <memory>
#include <functional>
#include <string>

#include "muduo-cpp11/base/macros.h"
#include "muduo-cpp11/base/timestamp.h"

namespace muduo_cpp11 {
namespace net {

class EventLoop;

///
/// A selectable I/O channel.
///
/// This class doesn't own the file descriptor.
/// The file descriptor could be a socket,
/// an eventfd, a timerfd, or a signalfd
class Channel {
 public:
  typedef std::function<void()> EventCallback;
  typedef std::function<void(Timestamp)> ReadEventCallback;

  Channel(EventLoop* loop, int fd);
  ~Channel();

  void HandleEvent(Timestamp receive_time);

  void set_read_callback(const ReadEventCallback& cb) {
    read_callback_ = cb;
  }

  void set_write_callback(const EventCallback& cb) {
    write_callback_ = cb;
  }

  void set_close_callback(const EventCallback& cb) {
    close_callback_ = cb;
  }

  void set_error_callback(const EventCallback& cb) {
    error_callback_ = cb;
  }

  /// Tie this channel to the owner object managed by shared_ptr,
  /// prevent the owner object being destroyed in HandleEvent.
  void Tie(const std::shared_ptr<void>&);

  int fd() const { return fd_; }
  int events() const { return events_; }
  void set_revents(int revt) { revents_ = revt; }  // used by pollers
  // int revents() const { return revents_; }
  bool IsNoneEvent() const { return events_ == kNoneEvent; }

  void EnableReading() { events_ |= kReadEvent; Update(); }
  void DisableReading() { events_ &= ~kReadEvent; Update(); }
  void EnableWriting() { events_ |= kWriteEvent; Update(); }
  void DisableWriting() { events_ &= ~kWriteEvent; Update(); }
  void DisableAll() { events_ = kNoneEvent; Update(); }
  bool IsWriting() const { return events_ & kWriteEvent; }

  // for Poller
  int index() { return index_; }
  void set_index(int idx) { index_ = idx; }

  // for debug
  std::string REventsToString() const;
  std::string EventsToString() const;

  void DoNotLogHup() { log_hup_ = false; }

  EventLoop* owner_loop() { return loop_; }
  void Remove();

 private:
  static std::string EventsToString(int fd, int ev);

  void Update();
  void HandleEventWithGuard(Timestamp receive_time);

  static const int kNoneEvent;
  static const int kReadEvent;
  static const int kWriteEvent;

  EventLoop* loop_;
  const int fd_;

  int events_;
  int revents_;  // it's the received event types of epoll or poll.
  int index_;  // used by Poller.
  bool log_hup_;

  std::weak_ptr<void> tie_;
  bool tied_;
  bool event_handling_;
  bool added_to_loop_;

  ReadEventCallback read_callback_;
  EventCallback write_callback_;
  EventCallback close_callback_;
  EventCallback error_callback_;

  DISABLE_COPY_AND_ASSIGN(Channel);
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_CHANNEL_H_
