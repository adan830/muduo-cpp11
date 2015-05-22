// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)

#include "muduo-cpp11/net/tcp_server.h"

#include <stdio.h>  // snprintf

#include <functional>
#include <string>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/net/acceptor.h"
#include "muduo-cpp11/net/event_loop.h"
#include "muduo-cpp11/net/event_loop_thread_pool.h"
#include "muduo-cpp11/net/sockets_ops.h"

using std::string;

namespace muduo_cpp11 {
namespace net {

TcpServer::TcpServer(EventLoop* loop,
                     const InetAddress& listen_addr,
                     const string& name,
                     Option option)
    : loop_(CHECK_NOTNULL(loop)),
      hostport_(listen_addr.ToIpPort()),
      name_(name),
      acceptor_(new Acceptor(loop, listen_addr, option == kReusePort)),
      thread_pool_(new EventLoopThreadPool(loop)),
      connection_callback_(DefaultConnectionCallback),
      message_callback_(DefaultMessageCallback),
      started_(ATOMIC_FLAG_INIT),
      next_conn_id_(1) {
  acceptor_->set_new_connection_callback(std::bind(&TcpServer::NewConnection,
                                                   this,
                                                   std::placeholders::_1,
                                                   std::placeholders::_2));
}

TcpServer::~TcpServer() {
  loop_->AssertInLoopThread();

  VLOG(1) << "TcpServer::~TcpServer [" << name_ << "] destructing";

  for (ConnectionMap::iterator it(connections_.begin());
       it != connections_.end();
       ++it) {
    TcpConnectionPtr conn = it->second;
    it->second.reset();
    conn->GetLoop()->RunInLoop(std::bind(&TcpConnection::ConnectDestroyed, conn));
    conn.reset();
  }
}

void TcpServer::set_thread_num(int num_threads) {
  CHECK_GE(num_threads, 0) << "num_threads should >= 0";
  thread_pool_->set_thread_num(num_threads);
}

void TcpServer::Start() {
  if (!started_.test_and_set()) {
    thread_pool_->Start(thread_init_callback_);
    CHECK(!acceptor_->listenning()) << "Acceptor should be listenning before TcpServer Start";
    loop_->RunInLoop(std::bind(&Acceptor::Listen, acceptor_.get()));
  }
}

void TcpServer::NewConnection(int sockfd, const InetAddress& peer_addr) {
  loop_->AssertInLoopThread();
  EventLoop* io_loop = thread_pool_->GetNextLoop();

  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", hostport_.c_str(), next_conn_id_);
  ++next_conn_id_;
  string conn_name = name_ + buf;

  LOG(INFO) << "TcpServer::NewConnection [" << name_
            << "] - new connection [" << conn_name
            << "] from " << peer_addr.ToIpPort();

  InetAddress local_addr(sockets::GetLocalAddr(sockfd));

  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(io_loop,
                                          conn_name,
                                          sockfd,
                                          local_addr,
                                          peer_addr));

  connections_[conn_name] = conn;

  conn->set_connection_callback(connection_callback_);
  conn->set_message_callback(message_callback_);
  conn->set_write_complete_callback(write_complete_callback_);
  conn->set_close_callback(std::bind(&TcpServer::RemoveConnection, this, std::placeholders::_1));  // FIXME: unsafe

  io_loop->RunInLoop(std::bind(&TcpConnection::ConnectEstablished, conn));
}

void TcpServer::RemoveConnection(const TcpConnectionPtr& conn) {
  // FIXME: unsafe
  loop_->RunInLoop(std::bind(&TcpServer::RemoveConnectionInLoop, this, conn));
}

void TcpServer::RemoveConnectionInLoop(const TcpConnectionPtr& conn) {
  loop_->AssertInLoopThread();
  LOG(INFO) << "TcpServer::removeConnectionInLoop [" << name_
            << "] - connection " << conn->name();
  size_t n = connections_.erase(conn->name());
  (void)n;
  assert(n == 1);
  EventLoop* io_loop = conn->GetLoop();
  io_loop->QueueInLoop(std::bind(&TcpConnection::ConnectDestroyed, conn));
}

}  // namespace net
}  // namespace muduo_cpp11
