// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)

#include "muduo-cpp11/net/acceptor.h"

#include <errno.h>
#include <fcntl.h>

// #include <sys/types.h>
// #include <sys/stat.h>

#include <functional>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/net/channel.h"
#include "muduo-cpp11/net/event_loop.h"
#include "muduo-cpp11/net/inet_address.h"
#include "muduo-cpp11/net/socket.h"
#include "muduo-cpp11/net/sockets_ops.h"

namespace muduo_cpp11 {
namespace net {

Acceptor::Acceptor(EventLoop* loop,
                   const InetAddress& listen_addr,
                   bool reuseport)
    : loop_(loop),
      accept_socket_ptr_(new Socket(sockets::CreateNonblockingOrDie())),
      accept_channel_ptr_(new Channel(loop, accept_socket_ptr_->fd())),
      listenning_(false),
      idle_fd_(::open("/dev/null", O_RDONLY | O_CLOEXEC)) {
  CHECK_GE(idle_fd_, 0) << "Failed to check idle_fd_";

  accept_socket_ptr_->set_reuseaddr(true);
  accept_socket_ptr_->set_reuseport(reuseport);
  accept_socket_ptr_->BindAddress(listen_addr);

  accept_channel_ptr_->set_read_callback(std::bind(&Acceptor::HandleRead, this));
}

Acceptor::~Acceptor() {
  accept_channel_ptr_->DisableAll();
  accept_channel_ptr_->Remove();
  ::close(idle_fd_);
}

void Acceptor::Listen() {
  loop_->AssertInLoopThread();
  listenning_ = true;
  accept_socket_ptr_->Listen();
  accept_channel_ptr_->EnableReading();
}

void Acceptor::HandleRead() {
  loop_->AssertInLoopThread();
  InetAddress peer_addr;

  // FIXME loop until no more
  int connfd = accept_socket_ptr_->Accept(&peer_addr);
  if (connfd >= 0) {
    // string hostport = peer_addr.ToIpPort();
    // VLOG(1) << "Accepts of " << hostport;
    if (new_connection_callback_) {
      new_connection_callback_(connfd, peer_addr);
    } else {
      sockets::Close(connfd);
    }
  } else {
    LOG(ERROR) << "Accept failed in Acceptor::HandleRead";

    // Read the section named "The special problem of
    // accept()ing when you can't" in libev's doc.
    // By Marc Lehmann, author of livev.
    if (errno == EMFILE) {
      ::close(idle_fd_);
      idle_fd_ = ::accept(accept_socket_ptr_->fd(), NULL, NULL);
      ::close(idle_fd_);
      idle_fd_ = ::open("/dev/null", O_RDONLY | O_CLOEXEC);
    }
  }
}

}  // namespace net
}  // namespace muduo_cpp11
