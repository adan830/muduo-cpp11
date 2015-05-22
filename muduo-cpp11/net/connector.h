// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_CPP11_NET_CONNECTOR_H_
#define MUDUO_CPP11_NET_CONNECTOR_H_

#include <atomic>
#include <memory>
#include <functional>

#include "muduo-cpp11/base/macros.h"
#include "muduo-cpp11/net/inet_address.h"

namespace muduo_cpp11 {
namespace net {

class Channel;
class EventLoop;

class Connector : public std::enable_shared_from_this<Connector> {
 public:
  typedef std::function<void (int sockfd)> NewConnectionCallback;

  Connector(EventLoop* loop, const InetAddress& server_addr);
  ~Connector();

  void set_new_connection_callback(const NewConnectionCallback& cb) {
    new_connection_callback_ = cb;
  }

  void Start();  // can be called in any thread
  void Restart();  // must be called in loop thread
  void Stop();  // can be called in any thread

  const InetAddress& server_address() const { return server_addr_; }

 private:
  enum States {
    kDisconnected,
    kConnecting,
    kConnected
  };

  static const int kMaxRetryDelayMs = 30 * 1000;
  static const int kInitRetryDelayMs = 500;

  void SetState(States s) {
    state_ = s;
  }

  void StartInLoop();
  void StopInLoop();
  void Connect();
  void Connecting(int sockfd);
  void HandleWrite();
  void HandleError();
  void Retry(int sockfd);
  int RemoveAndResetChannel();
  void ResetChannel();

 private:
  EventLoop* loop_;
  InetAddress server_addr_;
  std::atomic<bool> connect_;
  std::atomic<States> state_;
  std::unique_ptr<Channel> channel_;
  NewConnectionCallback new_connection_callback_;
  int retry_delay_ms_;

  DISABLE_COPY_AND_ASSIGN(Connector);
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_CONNECTOR_H_
