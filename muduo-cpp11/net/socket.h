// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_CPP11_NET_SOCKET_H_
#define MUDUO_CPP11_NET_SOCKET_H_

#include "muduo-cpp11/base/macros.h"

// struct tcp_info is in <netinet/tcp.h>
struct tcp_info;

namespace muduo_cpp11 {

///
/// TCP networking.
///
namespace net {

class InetAddress;

///
/// Wrapper of socket file descriptor.
///
/// It closes the sockfd when desctructs.
/// It's thread safe, all operations are delegated to OS.
class Socket {
 public:
  explicit Socket(int sockfd)
      : sockfd_(sockfd) {
  }

  // Socket(Socket&&)  // move constructor in C++11
  ~Socket();

  int fd() const { return sockfd_; }

  // return true if success.
  bool GetTcpInfo(struct tcp_info*) const;
  bool GetTcpInfoString(char* buf, int len) const;

  /// abort if address in use
  void BindAddress(const InetAddress& localaddr);
  /// abort if address in use
  void Listen();

  /// On success, returns a non-negative integer that is
  /// a descriptor for the accepted socket, which has been
  /// set to non-blocking and close-on-exec. *peeraddr is assigned.
  /// On error, -1 is returned, and *peeraddr is untouched.
  int Accept(InetAddress* peeraddr);

  void ShutdownWrite();

  ///
  /// Enable/disable TCP_NODELAY (disable/enable Nagle's algorithm).
  ///
  void set_tcpnodelay(bool on);

  ///
  /// Enable/disable SO_REUSEADDR
  ///
  void set_reuseaddr(bool on);

  ///
  /// Enable/disable SO_REUSEPORT
  ///
  void set_reuseport(bool on);

  ///
  /// Enable/disable SO_KEEPALIVE
  ///
  void set_keepalive(bool on);

 private:
  const int sockfd_;

  DISABLE_COPY_AND_ASSIGN(Socket);
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_SOCKET_H_
