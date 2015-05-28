// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_CPP11_NET_SOCKETS_OPS_H_
#define MUDUO_CPP11_NET_SOCKETS_OPS_H_

#include <arpa/inet.h>

namespace muduo_cpp11 {
namespace net {
namespace sockets {

///
/// Creates a non-blocking socket file descriptor,
/// abort if any error.
int CreateNonblockingOrDie();

int Connect(int sockfd, const struct sockaddr_in& addr);
void BindOrDie(int sockfd, const struct sockaddr_in& addr);
void ListenOrDie(int sockfd);
int Accept(int sockfd, struct sockaddr_in* addr);
ssize_t Read(int sockfd, void *buf, size_t count);
ssize_t Readv(int sockfd, const struct iovec *iov, int iovcnt);
ssize_t Write(int sockfd, const void *buf, size_t count);
void Close(int sockfd);
void ShutdownWrite(int sockfd);

void ToIpPort(char* buf, size_t size,
              const struct sockaddr_in& addr);
void ToIp(char* buf, size_t size,
          const struct sockaddr_in& addr);
void FromIpPort(const char* ip, uint16_t port,
                struct sockaddr_in* addr);

int GetSocketError(int sockfd);

const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr);
struct sockaddr* sockaddr_cast(struct sockaddr_in* addr);
const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr);
struct sockaddr_in* sockaddr_in_cast(struct sockaddr* addr);

struct sockaddr_in GetLocalAddr(int sockfd);
struct sockaddr_in GetPeerAddr(int sockfd);
bool IsSelfConnect(int sockfd);

}  // namespace sockets
}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_SOCKETS_OPS_H_
