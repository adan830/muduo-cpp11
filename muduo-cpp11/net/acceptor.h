// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_CPP11_NET_ACCEPTOR_H_
#define MUDUO_CPP11_NET_ACCEPTOR_H_

#include <memory>
#include <functional>

#include "muduo-cpp11/base/macros.h"

namespace muduo_cpp11 {
namespace net {

class Channel;
class EventLoop;
class InetAddress;
class Socket;

///
/// Acceptor of incoming TCP connections.
///
class Acceptor {
 public:
  typedef std::function<void (int sockfd, const InetAddress&)> NewConnectionCallback;

  Acceptor(EventLoop* loop, const InetAddress& listen_addr, bool reuseport);
  ~Acceptor();

  void set_new_connection_callback(const NewConnectionCallback& cb) {
    new_connection_callback_ = cb;
  }

  bool listenning() const {
    return listenning_;
  }

  void Listen();

 private:
  void HandleRead();

 private:
  EventLoop* loop_;
  NewConnectionCallback new_connection_callback_;

  std::unique_ptr<Socket> accept_socket_ptr_;
  std::unique_ptr<Channel> accept_channel_ptr_;

  bool listenning_;
  int idle_fd_;

  DISABLE_COPY_AND_ASSIGN(Acceptor);
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_ACCEPTOR_H_
