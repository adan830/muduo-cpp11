// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)

#include "muduo-cpp11/net/inet_address.h"

#include <assert.h>
#include <netdb.h>
#include <strings.h>  // bzero
#include <netinet/in.h>

#include <string>
#include <type_traits>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/net/endian.h"
#include "muduo-cpp11/net/sockets_ops.h"

using std::string;

// INADDR_ANY use (type)value casting.
#pragma GCC diagnostic ignored "-Wold-style-cast"
static const in_addr_t kInaddrAny = INADDR_ANY;
static const in_addr_t kInaddrLoopback = INADDR_LOOPBACK;
#pragma GCC diagnostic error "-Wold-style-cast"

//     /* Structure describing an Internet socket address.  */
//     struct sockaddr_in {
//         sa_family_t    sin_family; /* address family: AF_INET */
//         uint16_t       sin_port;   /* port in network byte order */
//         struct in_addr sin_addr;   /* internet address */
//     };

//     /* Internet address. */
//     typedef uint32_t in_addr_t;
//     struct in_addr {
//         in_addr_t       s_addr;     /* address in network byte order */
//     };

namespace muduo_cpp11 {
namespace net {

static_assert(sizeof(InetAddress) == sizeof(struct sockaddr_in),
              "sizeof(InetAddress) == sizeof(struct sockaddr_in)");

InetAddress::InetAddress(uint16_t port, bool loopback_only) {
  bzero(&addr_, sizeof addr_);
  addr_.sin_family = AF_INET;
  in_addr_t ip = loopback_only ? kInaddrLoopback : kInaddrAny;
  addr_.sin_addr.s_addr = sockets::HostToNetwork32(ip);
  addr_.sin_port = sockets::HostToNetwork16(port);
}

InetAddress::InetAddress(StringArg ip, uint16_t port) {
  bzero(&addr_, sizeof addr_);
  sockets::FromIpPort(ip.c_str(), port, &addr_);
}

string InetAddress::ToIpPort() const {
  char buf[32];
  sockets::ToIpPort(buf, sizeof buf, addr_);
  return buf;
}

string InetAddress::ToIp() const {
  char buf[32];
  sockets::ToIp(buf, sizeof buf, addr_);
  return buf;
}

uint16_t InetAddress::ToPort() const {
  return sockets::NetworkToHost16(addr_.sin_port);
}

static __thread char t_resolve_buffer[64 * 1024];

bool InetAddress::Resolve(StringArg hostname, InetAddress* out) {
  assert(out != NULL);
  struct hostent hent;
  struct hostent* he = NULL;
  int herrno = 0;
  bzero(&hent, sizeof(hent));

  int ret = gethostbyname_r(hostname.c_str(),
                            &hent,
                            t_resolve_buffer,
                            sizeof t_resolve_buffer,
                            &he,
                            &herrno);
  if (ret == 0 && he != NULL) {
    assert(he->h_addrtype == AF_INET && he->h_length == sizeof(uint32_t));
    out->addr_.sin_addr = *reinterpret_cast<struct in_addr*>(he->h_addr);
    return true;
  } else {
    if (ret) {
      LOG(ERROR) << "InetAddress::Resolve";
    }
    return false;
  }
}

}  // namespace net
}  // namespace muduo_cpp11
