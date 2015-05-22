// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)

#include "muduo-cpp11/net/tcp_connection.h"

#include <errno.h>

#include <functional>
#include <string>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/base/weak_callback.h"
#include "muduo-cpp11/net/channel.h"
#include "muduo-cpp11/net/event_loop.h"
#include "muduo-cpp11/net/socket.h"
#include "muduo-cpp11/net/sockets_ops.h"

using std::string;

namespace muduo_cpp11 {
namespace net {

void DefaultConnectionCallback(const TcpConnectionPtr& conn) {
  VLOG(1) << conn->local_address().ToIpPort() << " -> "
          << conn->peer_address().ToIpPort() << " is "
          << (conn->connected() ? "UP" : "DOWN");
  // do not call conn->forceClose(), because some
  // users want to register message callback only.
}

void DefaultMessageCallback(const TcpConnectionPtr&,
                            Buffer* buf,
                            Timestamp) {
  buf->RetrieveAll();
}

TcpConnection::TcpConnection(EventLoop* loop,
                             const string& name_arg,
                             int sockfd,
                             const InetAddress& local_addr,
                             const InetAddress& peer_addr)
    : loop_(CHECK_NOTNULL(loop)),
      name_(name_arg),
      state_(kConnecting),
      socket_(new Socket(sockfd)),
      channel_(new Channel(loop, sockfd)),
      local_addr_(local_addr),
      peer_addr_(peer_addr),
      high_watermark_(64 * 1024 * 1024) {
  channel_->set_read_callback(std::bind(&TcpConnection::HandleRead, this, std::placeholders::_1));
  channel_->set_write_callback(std::bind(&TcpConnection::HandleWrite, this));
  channel_->set_close_callback(std::bind(&TcpConnection::HandleClose, this));
  channel_->set_error_callback(std::bind(&TcpConnection::HandleError, this));
  DLOG(INFO) << "TcpConnection::ctor[" <<  name_ << "] at " << this
             << " fd=" << sockfd;
  socket_->set_keepalive(true);
}

TcpConnection::~TcpConnection() {
  DLOG(INFO) << "TcpConnection::dtor[" <<  name_ << "] at " << this
             << " fd=" << channel_->fd()
             << " state=" << StateToString();
  assert(state_ == kDisconnected);
}

bool TcpConnection::GetTcpInfo(struct tcp_info* tcpi) const {
  return socket_->GetTcpInfo(tcpi);
}

string TcpConnection::GetTcpInfoString() const {
  char buf[1024];
  buf[0] = '\0';
  socket_->GetTcpInfoString(buf, sizeof buf);
  return buf;
}

void TcpConnection::Send(const void* data, int len) {
  Send(StringPiece(static_cast<const char*>(data), len));
}

void TcpConnection::Send(const StringPiece& message) {
  if (state_ == kConnected) {
    if (loop_->IsInLoopThread()) {
      SendInLoop(message);
    } else {
      void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::SendInLoop;
      loop_->RunInLoop(std::bind(fp,
                                 this,  // FIXME
                                 message.as_string()));
                                 // std::forward<string>(message)));
    }
  }
}

// FIXME efficiency!!!
void TcpConnection::Send(Buffer* buf) {
  if (state_ == kConnected) {
    if (loop_->IsInLoopThread()) {
      SendInLoop(buf->Peek(), buf->ReadableBytes());
      buf->RetrieveAll();
    } else {
      void (TcpConnection::*fp)(const StringPiece& message) = &TcpConnection::SendInLoop;
      loop_->RunInLoop(std::bind(fp,
                                 this,  // FIXME
                                 buf->RetrieveAllAsString()));
                                 // std::forward<string>(message)));
    }
  }
}

void TcpConnection::SendInLoop(const StringPiece& message) {
  SendInLoop(message.data(), message.size());
}

void TcpConnection::SendInLoop(const void* data, size_t len) {
  loop_->AssertInLoopThread();

  ssize_t nwrote = 0;
  size_t remaining = len;
  bool fault_error = false;

  if (state_ == kDisconnected) {
    LOG(WARNING) << "disconnected, give up writing";
    return;
  }

  // if nothing in output queue, try writing directly
  if (!channel_->IsWriting() && output_buffer_.ReadableBytes() == 0) {
    nwrote = sockets::Write(channel_->fd(), data, len);
    if (nwrote >= 0) {
      remaining = len - nwrote;
      if (remaining == 0 && write_complete_callback_) {
        loop_->QueueInLoop(
            std::bind(write_complete_callback_, shared_from_this()));
      }
    } else {  // nwrote < 0
      nwrote = 0;
      if (errno != EWOULDBLOCK) {
        LOG(ERROR) << "TcpConnection::SendInLoop";
        if (errno == EPIPE || errno == ECONNRESET) {  // FIXME: any others?
          fault_error = true;
        }
      }
    }
  }

  assert(remaining <= len);

  if (!fault_error && remaining > 0) {
    size_t old_len = output_buffer_.ReadableBytes();
    if (old_len + remaining >= high_watermark_ &&
        old_len < high_watermark_ &&
        high_watermark_callback_) {
      loop_->QueueInLoop(std::bind(high_watermark_callback_,
                                   shared_from_this(),
                                   old_len + remaining));
    }

    output_buffer_.Append(static_cast<const char*>(data) + nwrote, remaining);

    if (!channel_->IsWriting()) {
      channel_->EnableWriting();
    }
  }
}

void TcpConnection::Shutdown() {
  // FIXME: use compare and swap
  if (state_ == kConnected) {
    set_state(kDisconnecting);
    // FIXME: shared_from_this()?
    loop_->RunInLoop(std::bind(&TcpConnection::ShutdownInLoop, this));
  }
}

void TcpConnection::ShutdownInLoop() {
  loop_->AssertInLoopThread();
  if (!channel_->IsWriting()) {
    // we are not writing
    socket_->ShutdownWrite();
  }
}

// void TcpConnection::shutdownAndForceCloseAfter(double seconds)
// {
//   // FIXME: use compare and swap
//   if (state_ == kConnected) {
//     setState(kDisconnecting);
//     loop_->runInLoop(
//       std::bind(&TcpConnection::shutdownAndForceCloseInLoop,
//                   this,
//                   seconds));
//   }
// }

// void TcpConnection::shutdownAndForceCloseInLoop(double seconds)
// {
//   loop_->assertInLoopThread();
//   if (!channel_->isWriting()) {
//     // we are not writing
//     socket_->shutdownWrite();
//   }
//   loop_->runAfter(
//       seconds,
//       makeWeakCallback(shared_from_this(),
//                        &TcpConnection::forceCloseInLoop));
// }

void TcpConnection::ForceClose() {
  // FIXME: use compare and swap
  if (state_ == kConnected || state_ == kDisconnecting) {
    set_state(kDisconnecting);
    loop_->QueueInLoop(std::bind(&TcpConnection::ForceCloseInLoop, shared_from_this()));
  }
}

void TcpConnection::ForceCloseWithDelay(double seconds) {
  if (state_ == kConnected || state_ == kDisconnecting) {
    set_state(kDisconnecting);
    // not ForceCloseInLoop to avoid race condition
    loop_->RunAfter(seconds, MakeWeakCallback(shared_from_this(), &TcpConnection::ForceClose));
  }
}

void TcpConnection::ForceCloseInLoop() {
  loop_->AssertInLoopThread();
  if (state_ == kConnected || state_ == kDisconnecting) {
    // as if we received 0 byte in handleRead();
    HandleClose();
  }
}

const char* TcpConnection::StateToString() const {
  switch (state_) {
    case kDisconnected:
      return "kDisconnected";
    case kConnecting:
      return "kConnecting";
    case kConnected:
      return "kConnected";
    case kDisconnecting:
      return "kDisconnecting";
    default:
      return "unknown state";
  }
}

void TcpConnection::SetTcpNoDelay(bool on) {
  socket_->set_tcpnodelay(on);
}

void TcpConnection::ConnectEstablished() {
  loop_->AssertInLoopThread();
  assert(state_ == kConnecting);
  set_state(kConnected);
  channel_->Tie(shared_from_this());
  channel_->EnableReading();

  connection_callback_(shared_from_this());
}

void TcpConnection::ConnectDestroyed() {
  loop_->AssertInLoopThread();
  if (state_ == kConnected) {
    set_state(kDisconnected);
    channel_->DisableAll();
    connection_callback_(shared_from_this());
  }
  channel_->Remove();
}

void TcpConnection::HandleRead(Timestamp receive_time) {
  loop_->AssertInLoopThread();
  int saved_errno = 0;
  ssize_t n = input_buffer_.ReadFd(channel_->fd(), &saved_errno);
  if (n > 0) {
    message_callback_(shared_from_this(), &input_buffer_, receive_time);
  } else if (n == 0) {
    HandleClose();
  } else {
    errno = saved_errno;
    LOG(ERROR) << "TcpConnection::HandleRead";
    HandleError();
  }
}

void TcpConnection::HandleWrite() {
  loop_->AssertInLoopThread();
  if (channel_->IsWriting()) {
    ssize_t n = sockets::Write(channel_->fd(),
                               output_buffer_.Peek(),
                               output_buffer_.ReadableBytes());
    if (n > 0) {
      output_buffer_.Retrieve(n);
      if (output_buffer_.ReadableBytes() == 0) {
        channel_->DisableWriting();
        if (write_complete_callback_) {
          loop_->QueueInLoop(std::bind(write_complete_callback_, shared_from_this()));
        }
        if (state_ == kDisconnecting) {
          ShutdownInLoop();
        }
      }
    } else {
      LOG(ERROR) << "TcpConnection::handleWrite";
      // if (state_ == kDisconnecting) {
      //   shutdownInLoop();
      // }
    }
  } else {
    VLOG(1) << "Connection fd = " << channel_->fd()
            << " is down, no more writing";
  }
}

void TcpConnection::HandleClose() {
  loop_->AssertInLoopThread();
  VLOG(1) << "fd = " << channel_->fd() << " state = " << state_;
  assert(state_ == kConnected || state_ == kDisconnecting);
  // we don't close fd, leave it to dtor, so we can find leaks easily.
  set_state(kDisconnected);
  channel_->DisableAll();

  TcpConnectionPtr guard_this(shared_from_this());
  connection_callback_(guard_this);
  // must be the last line
  close_callback_(guard_this);
}

void TcpConnection::HandleError() {
  int err = sockets::GetSocketError(channel_->fd());
  LOG(ERROR) << "TcpConnection::HandleError [" << name_
             << "] - SO_ERROR = " << err << " " << strerror_tl(err);
}

}  // namespace net
}  // namespace muduo_cpp11
