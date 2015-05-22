// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_CPP11_NET_TCP_CLIENT_H_
#define MUDUO_CPP11_NET_TCP_CLIENT_H_

#include <atomic>
#include <mutex>
#include <string>

#include "muduo-cpp11/base/macros.h"
#include "muduo-cpp11/net/tcp_connection.h"

namespace muduo_cpp11 {
namespace net {

class Connector;
typedef std::shared_ptr<Connector> ConnectorPtr;

class TcpClient {
 public:
  // TcpClient(EventLoop* loop);
  // TcpClient(EventLoop* loop, const std::string& host, uint16_t port);
  TcpClient(EventLoop* loop,
            const InetAddress& serverAddr,
            const std::string& name);
  ~TcpClient();

  void Connect();
  void Disconnect();
  void Stop();

  TcpConnectionPtr connection() const {
    std::lock_guard<std::mutex> lock(mutex_);
    return connection_;
  }

  EventLoop* GetLoop() const { return loop_; }
  bool retry() const { return retry_; }
  void EnableRetry() { retry_ = true; }

  const std::string& name() const { return name_; }

  /// Set connection callback.
  /// Not thread safe.
  void set_connection_callback(const ConnectionCallback& cb) {
    connection_callback_ = cb;
  }

  /// Set message callback.
  /// Not thread safe.
  void set_message_callback(const MessageCallback& cb) {
    message_callback_ = cb;
  }

  /// Set write complete callback.
  /// Not thread safe.
  void set_write_complete_callback(const WriteCompleteCallback& cb) {
    write_complete_callback_ = cb;
  }

 private:
  /// Not thread safe, but in loop
  void NewConnection(int sockfd);
  /// Not thread safe, but in loop
  void RemoveConnection(const TcpConnectionPtr& conn);

 private:
  EventLoop* loop_;
  ConnectorPtr connector_;  // avoid revealing Connector
  const std::string name_;

  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;

  std::atomic<bool> retry_;
  std::atomic<bool> connect_;

  // always in loop thread
  int next_conn_id_;
  mutable std::mutex mutex_;
  TcpConnectionPtr connection_;  // @GuardedBy mutex_

  DISABLE_COPY_AND_ASSIGN(TcpClient);
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_TCP_CLIENT_H_
