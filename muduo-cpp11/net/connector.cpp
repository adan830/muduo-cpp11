// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo-cpp11/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)
//

#include "muduo-cpp11/net/connector.h"

#include <assert.h>
#include <errno.h>

#include <algorithm>
#include <functional>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/net/channel.h"
#include "muduo-cpp11/net/event_loop.h"
#include "muduo-cpp11/net/sockets_ops.h"

namespace muduo_cpp11 {
namespace net {

const int Connector::kMaxRetryDelayMs;

Connector::Connector(EventLoop* loop, const InetAddress& server_addr)
    : loop_(loop),
      server_addr_(server_addr),
      connect_(false),
      state_(kDisconnected),
      retry_delay_ms_(kInitRetryDelayMs) {
  DLOG(INFO) << "ctor[" << this << "]";
}

Connector::~Connector() {
  DLOG(INFO) << "dtor[" << this << "]";
  assert(!channel_);
}

void Connector::Start() {
  connect_ = true;
  loop_->RunInLoop(std::bind(&Connector::StartInLoop, this));  // FIXME: unsafe
}

void Connector::StartInLoop() {
  loop_->AssertInLoopThread();
  assert(state_ == kDisconnected);
  if (connect_) {
    Connect();
  } else {
    DLOG(INFO) << "do not connect";
  }
}

void Connector::Stop() {
  connect_ = false;
  loop_->QueueInLoop(std::bind(&Connector::StopInLoop, this));  // FIXME: unsafe
  // FIXME: cancel timer
}

void Connector::StopInLoop() {
  loop_->AssertInLoopThread();
  if (state_ == kConnecting) {
    SetState(kDisconnected);
    int sockfd = RemoveAndResetChannel();
    Retry(sockfd);
  }
}

void Connector::Connect() {
  int sockfd = sockets::CreateNonblockingOrDie();
  int ret = sockets::Connect(sockfd, server_addr_.GetSockAddrInet());
  int saved_errno = (ret == 0) ? 0 : errno;
  switch (saved_errno) {
    case 0:
    case EINPROGRESS:
    case EINTR:
    case EISCONN:
      Connecting(sockfd);
      break;

    case EAGAIN:
    case EADDRINUSE:
    case EADDRNOTAVAIL:
    case ECONNREFUSED:
    case ENETUNREACH:
      Retry(sockfd);
      break;

    case EACCES:
    case EPERM:
    case EAFNOSUPPORT:
    case EALREADY:
    case EBADF:
    case EFAULT:
    case ENOTSOCK:
      LOG(ERROR) << "connect error in Connector::startInLoop " << saved_errno;
      sockets::Close(sockfd);
      break;

    default:
      LOG(ERROR) << "Unexpected error in Connector::startInLoop " << saved_errno;
      sockets::Close(sockfd);
      // connectErrorCallback_();
      break;
  }
}

void Connector::Restart() {
  loop_->AssertInLoopThread();
  SetState(kDisconnected);
  retry_delay_ms_ = kInitRetryDelayMs;
  connect_ = true;
  StartInLoop();
}

void Connector::Connecting(int sockfd) {
  SetState(kConnecting);
  assert(!channel_);
  channel_.reset(new Channel(loop_, sockfd));
  channel_->set_write_callback(std::bind(&Connector::HandleWrite, this));  // FIXME: unsafe
  channel_->set_error_callback(std::bind(&Connector::HandleError, this));  // FIXME: unsafe

  // channel_->Tie(shared_from_this()); is not working,
  // as channel_ is not managed by shared_ptr
  channel_->EnableWriting();
}

int Connector::RemoveAndResetChannel() {
  channel_->DisableAll();
  channel_->Remove();
  int sockfd = channel_->fd();
  // Can't reset channel_ here, because we are inside Channel::handleEvent
  loop_->QueueInLoop(std::bind(&Connector::ResetChannel, this));  // FIXME: unsafe
  return sockfd;
}

void Connector::ResetChannel() {
  channel_.reset();
}

void Connector::HandleWrite() {
  VLOG(1) << "Connector::HandleWrite " << state_;

  if (state_ == kConnecting) {
    int sockfd = RemoveAndResetChannel();
    int err = sockets::GetSocketError(sockfd);
    if (err) {
      LOG(WARNING) << "Connector::HandleWrite - SO_ERROR = "
                   << err << " " << strerror_tl(err);
      Retry(sockfd);
    } else if (sockets::IsSelfConnect(sockfd)) {
      LOG(WARNING) << "Connector::HandleWrite - Self connect";
      Retry(sockfd);
    } else {
      SetState(kConnected);
      if (connect_) {
        new_connection_callback_(sockfd);
      } else {
        sockets::Close(sockfd);
      }
    }
  } else {
    // what happened?
    assert(state_ == kDisconnected);
  }
}

void Connector::HandleError() {
  LOG(ERROR) << "Connector::handleError state=" << state_;
  if (state_ == kConnecting) {
    int sockfd = RemoveAndResetChannel();
    int err = sockets::GetSocketError(sockfd);
    VLOG(1) << "SO_ERROR = " << err << " " << strerror_tl(err);
    Retry(sockfd);
  }
}

void Connector::Retry(int sockfd) {
  sockets::Close(sockfd);
  SetState(kDisconnected);

  if (connect_) {
    LOG(INFO) << "Connector::Retry - Retry connecting to " << server_addr_.ToIpPort()
             << " in " << retry_delay_ms_ << " milliseconds. ";
    loop_->RunAfter(retry_delay_ms_/1000.0,
                    std::bind(&Connector::StartInLoop, shared_from_this()));
    retry_delay_ms_ = std::min(retry_delay_ms_ * 2, kMaxRetryDelayMs);
  } else {
    DLOG(INFO) << "do not connect";
  }
}

}  // namespace net
}  // namespace muduo_cpp11
