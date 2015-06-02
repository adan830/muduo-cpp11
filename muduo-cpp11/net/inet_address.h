// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_CPP11_NET_INET_ADDRESS_H_
#define MUDUO_CPP11_NET_INET_ADDRESS_H_

#include <netinet/in.h>

#include <string>

#include "muduo-cpp11/base/string_piece.h"

namespace muduo_cpp11 {
namespace net {

///
/// Wrapper of sockaddr_in.
///
/// This is an POD interface class.
class InetAddress {
 public:
  /// Constructs an endpoint with given port number.
  /// Mostly used in TcpServer listening.
  explicit InetAddress(uint16_t port = 0, bool loopback_only = false);

  /// Constructs an endpoint with given ip and port.
  /// @c ip should be "1.2.3.4"
  InetAddress(StringArg ip, uint16_t port);

  /// Constructs an endpoint with given struct @c sockaddr_in
  /// Mostly used when accepting new connections
  InetAddress(const struct sockaddr_in& addr)
      : addr_(addr) {
  }

  std::string ToIp() const;
  std::string ToIpPort() const;
  uint16_t ToPort() const;

  // default copy/assignment are Okay

  const struct sockaddr_in& GetSockAddrInet() const { return addr_; }
  void SetSockAddrInet(const struct sockaddr_in& addr) { addr_ = addr; }

  uint32_t IpNetEndian() const { return addr_.sin_addr.s_addr; }
  uint16_t PortNetEndian() const { return addr_.sin_port; }

#if !defined(__MACH__) && !defined(__ANDROID_API__)
  // resolve hostname to IP address, not changing port or sin_family
  // return true on success.
  // thread safe
  static bool Resolve(StringArg hostname, InetAddress* result);
  // static std::vector<InetAddress> ResolveAll(const char* hostname, uint16_t port = 0);
#endif

 private:
  struct sockaddr_in addr_;
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_INET_ADDRESS_H_
