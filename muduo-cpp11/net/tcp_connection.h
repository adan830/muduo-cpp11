// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_CPP11_NET_TCP_CONNECTION_H_
#define MUDUO_CPP11_NET_TCP_CONNECTION_H_

#include <atomic>
#include <memory>
#include <string>

#include <boost/any.hpp>

#include "muduo-cpp11/base/macros.h"
#include "muduo-cpp11/base/string_piece.h"
#include "muduo-cpp11/net/callbacks.h"
#include "muduo-cpp11/net/buffer.h"
#include "muduo-cpp11/net/inet_address.h"

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace muduo_cpp11 {
namespace net {

class Channel;
class EventLoop;
class Socket;

///
/// TCP connection, for both client and server usage.
///
/// This is an interface class, so don't expose too much details.
class TcpConnection : public std::enable_shared_from_this<TcpConnection> {
 public:
  /// Constructs a TcpConnection with a connected sockfd
  ///
  /// User should not create this object.
  TcpConnection(EventLoop* loop,
                const std::string& name,
                int sockfd,
                const InetAddress& local_addr,
                const InetAddress& peer_addr);
  ~TcpConnection();

  EventLoop* GetLoop() const {
    return loop_;
  }

  const std::string& name() const {
    return name_;
  }

  const InetAddress& local_address() const {
    return local_addr_;
  }

  const InetAddress& peer_address() const {
    return peer_addr_;
  }

  bool connected() const {
    return state_ == kConnected;
  }

  bool disconnected() const {
    return state_ == kDisconnected;
  }

  // return true if success.
  bool GetTcpInfo(struct tcp_info*) const;
  std::string GetTcpInfoString() const;

  void Send(const void* message, int len);
  void Send(const StringPiece& message);
  void Send(Buffer* message);  // this one will swap data

  // NOT thread safe, no simultaneous calling
  void Shutdown();

  // NOT thread safe, no simultaneous calling
  // void shutdownAndForceCloseAfter(double seconds);

  void ForceClose();
  void ForceCloseWithDelay(double seconds);
  void SetTcpNoDelay(bool on);

  void set_context(const boost::any& context) {
    context_ = context;
  }

  const boost::any& context() const {
    return context_;
  }

  boost::any* mutable_context() {
    return &context_;
  }

  void set_connection_callback(const ConnectionCallback& cb) {
    connection_callback_ = cb;
  }

  void set_message_callback(const MessageCallback& cb) {
    message_callback_ = cb;
  }

  void set_write_complete_callback(const WriteCompleteCallback& cb) {
    write_complete_callback_ = cb;
  }

  void set_high_watermark_callback(const HighWaterMarkCallback& cb,
                                    size_t high_watermark) {
    high_watermark_callback_ = cb;
    high_watermark_ = high_watermark;
  }

  /// Advanced interface
  Buffer* input_buffer() {
    return &input_buffer_;
  }

  Buffer* output_buffer() {
    return &output_buffer_;
  }

  /// Internal use only.
  void set_close_callback(const CloseCallback& cb) {
    close_callback_ = cb;
  }

  // called when TcpServer accepts a new connection
  void ConnectEstablished();  // should be called only once

  // called when TcpServer has removed me from its map
  void ConnectDestroyed();  // should be called only once

 private:
  enum StateE { kDisconnected, kConnecting, kConnected, kDisconnecting };

  void HandleRead(Timestamp receive_time);
  void HandleWrite();
  void HandleClose();
  void HandleError();

  // void SendInLoop(std::string&& message);
  void SendInLoop(const StringPiece& message);
  void SendInLoop(const void* message, size_t len);
  void ShutdownInLoop();
  // void ShutdownAndForceCloseInLoop(double seconds);
  void ForceCloseInLoop();

  void set_state(StateE s) { state_ = s; }
  const char* StateToString() const;

  EventLoop* loop_;
  const std::string name_;
  std::atomic<StateE> state_;

  // we don't expose those classes to client.
  std::unique_ptr<Socket> socket_;
  std::unique_ptr<Channel> channel_;
  const InetAddress local_addr_;
  const InetAddress peer_addr_;

  ConnectionCallback connection_callback_;
  MessageCallback message_callback_;
  WriteCompleteCallback write_complete_callback_;
  HighWaterMarkCallback high_watermark_callback_;
  CloseCallback close_callback_;

  size_t high_watermark_;
  Buffer input_buffer_;
  Buffer output_buffer_;  // FIXME: use list<Buffer> as output buffer.

  boost::any context_;

  // FIXME: creationTime_, lastReceiveTime_
  //        bytesReceived_, bytesSent_

  DISABLE_COPY_AND_ASSIGN(TcpConnection);
};

typedef std::shared_ptr<TcpConnection> TcpConnectionPtr;

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_TCP_CONNECTION_H_
