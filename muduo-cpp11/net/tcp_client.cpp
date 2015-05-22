// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//

#include "muduo-cpp11/net/tcp_client.h"

#include <stdio.h>  // snprintf

#include <functional>
#include <string>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/net/connector.h"
#include "muduo-cpp11/net/event_loop.h"
#include "muduo-cpp11/net/sockets_ops.h"

using std::string;

namespace muduo_cpp11 {
namespace net {

// TcpClient::TcpClient(EventLoop* loop)
//     : loop_(loop) {
// }

// TcpClient::TcpClient(EventLoop* loop, const string& host, uint16_t port)
//     : loop_(CHECK_NOTNULL(loop)),
//       server_addr_(host, port) {
// }

namespace detail {

void RemoveConnection(EventLoop* loop, const TcpConnectionPtr& conn) {
  loop->QueueInLoop(std::bind(&TcpConnection::ConnectDestroyed, conn));
}

void RemoveConnector(const ConnectorPtr& connector) {
}

}  // namespace detail

TcpClient::TcpClient(EventLoop* loop,
                     const InetAddress& server_addr,
                     const string& name)
    : loop_(CHECK_NOTNULL(loop)),
      connector_(new Connector(loop, server_addr)),
      name_(name),
      connection_callback_(DefaultConnectionCallback),
      message_callback_(DefaultMessageCallback),
      retry_(false),
      connect_(true),
      next_conn_id_(1) {
  connector_->set_new_connection_callback(std::bind(&TcpClient::NewConnection, this, std::placeholders::_1));
  // FIXME set_connect_failed_callback
  LOG(INFO) << "TcpClient::TcpClient[" << name_ << "] - connector " << connector_.get();
}

TcpClient::~TcpClient() {
  LOG(INFO) << "TcpClient::~TcpClient[" << name_ << "] - connector " << connector_.get();

  TcpConnectionPtr conn;
  bool unique = false;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    unique = connection_.unique();
    conn = connection_;
  }

  if (conn) {
    assert(loop_ == conn->GetLoop());
    // FIXME: not 100% safe, if we are in different thread
    CloseCallback cb = std::bind(&detail::RemoveConnection, loop_, std::placeholders::_1);
    loop_->RunInLoop(std::bind(&TcpConnection::set_close_callback, conn, cb));
    if (unique) {
      conn->ForceClose();
    }
  } else {
    connector_->Stop();
    // FIXME: HACK
    loop_->RunAfter(1, std::bind(&detail::RemoveConnector, connector_));
  }
}

void TcpClient::Connect() {
  // FIXME: check state
  LOG(INFO) << "TcpClient::connect[" << name_ << "] - connecting to "
            << connector_->server_address().ToIpPort();
  connect_ = true;
  connector_->Start();
}

void TcpClient::Disconnect() {
  connect_ = false;

  {
    std::lock_guard<std::mutex> lock(mutex_);
    if (connection_) {
      connection_->Shutdown();
    }
  }
}

void TcpClient::Stop() {
  connect_ = false;
  connector_->Stop();
}

void TcpClient::NewConnection(int sockfd) {
  loop_->AssertInLoopThread();
  InetAddress peer_addr(sockets::GetPeerAddr(sockfd));
  char buf[32];
  snprintf(buf, sizeof buf, ":%s#%d", peer_addr.ToIpPort().c_str(), next_conn_id_);
  ++next_conn_id_;
  string conn_name = name_ + buf;

  InetAddress local_addr(sockets::GetLocalAddr(sockfd));
  // FIXME poll with zero timeout to double confirm the new connection
  // FIXME use make_shared if necessary
  TcpConnectionPtr conn(new TcpConnection(loop_,
                                          conn_name,
                                          sockfd,
                                          local_addr,
                                          peer_addr));

  conn->set_connection_callback(connection_callback_);
  conn->set_message_callback(message_callback_);
  conn->set_write_complete_callback(write_complete_callback_);
  conn->set_close_callback(std::bind(&TcpClient::RemoveConnection, this, std::placeholders::_1));  // FIXME: unsafe

  {
    std::lock_guard<std::mutex> lock(mutex_);
    connection_ = conn;
  }

  conn->ConnectEstablished();
}

void TcpClient::RemoveConnection(const TcpConnectionPtr& conn) {
  loop_->AssertInLoopThread();
  assert(loop_ == conn->GetLoop());

  {
    std::lock_guard<std::mutex> lock(mutex_);
    assert(connection_ == conn);
    connection_.reset();
  }

  loop_->QueueInLoop(std::bind(&TcpConnection::ConnectDestroyed, conn));

  if (retry_ && connect_) {
    LOG(INFO) << "TcpClient::connect[" << name_ << "] - Reconnecting to "
              << connector_->server_address().ToIpPort();
    connector_->Restart();
  }
}

}  // namespace net
}  // namespace muduo_cpp11
