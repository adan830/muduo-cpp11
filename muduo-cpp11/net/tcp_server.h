// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_CPP11_NET_TCP_SERVER_H_
#define MUDUO_CPP11_NET_TCP_SERVER_H_

#include <atomic>
#include <map>
#include <memory>
#include <string>

#include "muduo-cpp11/base/macros.h"
#include "muduo-cpp11/net/tcp_connection.h"

namespace muduo_cpp11 {
namespace net {

class Acceptor;
class EventLoop;
class EventLoopThreadPool;

///
/// TCP server, supports single-threaded and thread-pool models.
///
/// This is an interface class, so don't expose too much details.
class TcpServer {
 public:
  typedef std::function<void(EventLoop*)> ThreadInitCallback;

  enum Option {
    kNoReusePort,
    kReusePort,
  };

  TcpServer(EventLoop* loop,
            const InetAddress& listen_addr,
            const std::string& name,
            Option option = kNoReusePort);

  ~TcpServer();

  const std::string& hostport() const {
    return hostport_;
  }

  const std::string& name() const {
    return name_;
  }

  EventLoop* GetLoop() const {
    return loop_;
  }

  /// Set the number of threads for handling input.
  ///
  /// Always accepts new connection in loop's thread.
  /// Must be called before @c Start
  /// @param numThreads
  /// - 0 means all I/O in loop's thread, no thread will created.
  ///   this is the default value.
  /// - 1 means all I/O in another thread.
  /// - N means a thread pool with N threads, new connections
  ///   are assigned on a round-robin basis.
  void set_thread_num(int num_threads);

  void set_thread_init_callback(const ThreadInitCallback& cb) {
    thread_init_callback_ = cb;
  }

  /// valid after calling Start()
  std::shared_ptr<EventLoopThreadPool> thread_pool() {
    return thread_pool_;
  }

  /// Starts the server if it's not listenning.
  ///
  /// It's harmless to call it multiple times.
  /// Thread safe.
  void Start();

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
  void NewConnection(int sockfd, const InetAddress& peer_addr);

  /// Thread safe.
  void RemoveConnection(const TcpConnectionPtr& conn);

  /// Not thread safe, but in loop
  void RemoveConnectionInLoop(const TcpConnectionPtr& conn);

 private:
  typedef std::map<std::string, TcpConnectionPtr> ConnectionMap;

  EventLoop* loop_;  // the acceptor loop
  const std::string hostport_;
  const std::string name_;

  std::unique_ptr<Acceptor> acceptor_;  // avoid revealing Acceptor
  std::shared_ptr<EventLoopThreadPool> thread_pool_;

  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;

  ThreadInitCallback thread_init_callback_;
  std::atomic_flag started_;
  
  // always in loop thread
  int next_conn_id_;
  ConnectionMap connections_;

  DISABLE_COPY_AND_ASSIGN(TcpServer);
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_TCP_SERVER_H_
