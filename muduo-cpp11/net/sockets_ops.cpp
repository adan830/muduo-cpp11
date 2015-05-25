// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)

#include "muduo-cpp11/net/sockets_ops.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>  // snprintf
#include <strings.h>  // bzero
#include <sys/socket.h>
#include <unistd.h>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/base/type_conversion.h"
#include "muduo-cpp11/net/endian.h"

namespace {

typedef struct sockaddr SA;

#if VALGRIND || defined (NO_ACCEPT4)
void SetNonBlockAndCloseOnExec(int sockfd) {
  // non-block
  int flags = ::fcntl(sockfd, F_GETFL, 0);
  flags |= O_NONBLOCK;
  int ret = ::fcntl(sockfd, F_SETFL, flags);
  // FIXME check

  // close-on-exec
  flags = ::fcntl(sockfd, F_GETFD, 0);
  flags |= FD_CLOEXEC;
  ret = ::fcntl(sockfd, F_SETFD, flags);
  // FIXME check

  (void)ret;
}
#endif

}  // namespace

namespace muduo_cpp11 {
namespace net {
namespace sockets {

const struct sockaddr* sockaddr_cast(const struct sockaddr_in* addr) {
  return static_cast<const struct sockaddr*>(implicit_cast<const void*>(addr));
}

struct sockaddr* sockaddr_cast(struct sockaddr_in* addr) {
  return static_cast<struct sockaddr*>(implicit_cast<void*>(addr));
}

const struct sockaddr_in* sockaddr_in_cast(const struct sockaddr* addr) {
  return static_cast<const struct sockaddr_in*>(implicit_cast<const void*>(addr));
}

struct sockaddr_in* sockaddr_in_cast(struct sockaddr* addr) {
  return static_cast<struct sockaddr_in*>(implicit_cast<void*>(addr));
}

int CreateNonblockingOrDie() {
#if VALGRIND
  int sockfd = ::socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
  if (sockfd < 0) {
    LOG(FATAL) << "sockets::CreateNonblockingOrDie";
  }

  SetNonBlockAndCloseOnExec(sockfd);
#else
  int sockfd = ::socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC, IPPROTO_TCP);
  if (sockfd < 0) {
    LOG(FATAL) << "sockets::CreateNonblockingOrDie";
  }
#endif
  return sockfd;
}

void BindOrDie(int sockfd, const struct sockaddr_in& addr) {
  int ret = ::bind(sockfd,
                   sockaddr_cast(&addr),
                   static_cast<socklen_t>(sizeof addr));
  if (ret < 0) {
    LOG(FATAL) << "sockets::BindOrDie";
  }
}

void ListenOrDie(int sockfd) {
  int ret = ::listen(sockfd, SOMAXCONN);
  if (ret < 0) {
    LOG(FATAL) << "sockets::ListenOrDie";
  }
}

int Accept(int sockfd, struct sockaddr_in* addr) {
  socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
#if VALGRIND  || defined (NO_ACCEPT4)
  int connfd = ::accept(sockfd, sockaddr_cast(addr), &addrlen);
  SetNonBlockAndCloseOnExec(connfd);
#else
  int connfd = ::accept4(sockfd,
                         sockaddr_cast(addr),
                         &addrlen,
                         SOCK_NONBLOCK | SOCK_CLOEXEC);
#endif
  if (connfd < 0) {
    int saved_errno = errno;
    LOG(ERROR) << "Socket::Accept";
    switch (saved_errno) {
      case EAGAIN:
      case ECONNABORTED:
      case EINTR:
      case EPROTO:  // ???
      case EPERM:
      case EMFILE:  // per-process lmit of open file desctiptor ???
        // expected errors
        errno = saved_errno;
        break;
      case EBADF:
      case EFAULT:
      case EINVAL:
      case ENFILE:
      case ENOBUFS:
      case ENOMEM:
      case ENOTSOCK:
      case EOPNOTSUPP:
        // unexpected errors
        LOG(FATAL) << "unexpected error of ::accept " << saved_errno;
        break;
      default:
        LOG(FATAL) << "unknown error of ::accept " << saved_errno;
        break;
    }
  }
  return connfd;
}

int Connect(int sockfd, const struct sockaddr_in& addr) {
  return ::connect(sockfd,
                   sockaddr_cast(&addr),
                   static_cast<socklen_t>(sizeof addr));
}

ssize_t Read(int sockfd, void *buf, size_t count) {
  return ::read(sockfd, buf, count);
}

ssize_t Readv(int sockfd, const struct iovec *iov, int iovcnt) {
  return ::readv(sockfd, iov, iovcnt);
}

ssize_t Write(int sockfd, const void *buf, size_t count) {
  return ::write(sockfd, buf, count);
}

void Close(int sockfd) {
  if (::close(sockfd) < 0) {
    LOG(ERROR) << "sockets::Close";
  }
}

void ShutdownWrite(int sockfd) {
  if (::shutdown(sockfd, SHUT_WR) < 0) {
    LOG(ERROR) << "sockets::ShutdownWrite";
  }
}

void ToIpPort(char* buf,
              size_t size,
              const struct sockaddr_in& addr) {
  assert(size >= INET_ADDRSTRLEN);
  ::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(size));
  size_t end = ::strlen(buf);
  uint16_t port = NetworkToHost16(addr.sin_port);
  assert(size > end);
  snprintf(buf + end, size - end, ":%u", port);
}

void ToIp(char* buf,
          size_t size,
          const struct sockaddr_in& addr) {
  assert(size >= INET_ADDRSTRLEN);
  ::inet_ntop(AF_INET, &addr.sin_addr, buf, static_cast<socklen_t>(size));
}

void FromIpPort(const char* ip,
                uint16_t port,
                struct sockaddr_in* addr) {
  addr->sin_family = AF_INET;
  addr->sin_port = HostToNetwork16(port);
  if (::inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
    LOG(ERROR) << "sockets::FromIpPort";
  }
}

int GetSocketError(int sockfd) {
  int optval;
  socklen_t optlen = static_cast<socklen_t>(sizeof optval);

  if (::getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &optval, &optlen) < 0) {
    return errno;
  } else {
    return optval;
  }
}

struct sockaddr_in GetLocalAddr(int sockfd) {
  struct sockaddr_in localaddr;
  bzero(&localaddr, sizeof localaddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof localaddr);
  if (::getsockname(sockfd, sockaddr_cast(&localaddr), &addrlen) < 0) {
    LOG(ERROR) << "sockets::GetLocalAddr";
  }
  return localaddr;
}

struct sockaddr_in GetPeerAddr(int sockfd) {
  struct sockaddr_in peeraddr;
  bzero(&peeraddr, sizeof peeraddr);
  socklen_t addrlen = static_cast<socklen_t>(sizeof peeraddr);
  if (::getpeername(sockfd, sockaddr_cast(&peeraddr), &addrlen) < 0) {
    LOG(ERROR) << "sockets::GetPeerAddr";
  }
  return peeraddr;
}

bool IsSelfConnect(int sockfd) {
  struct sockaddr_in localaddr = GetLocalAddr(sockfd);
  struct sockaddr_in peeraddr = GetPeerAddr(sockfd);
  return localaddr.sin_port == peeraddr.sin_port
      && localaddr.sin_addr.s_addr == peeraddr.sin_addr.s_addr;
}

}  // namespace sockets
}  // namespace net
}  // namespace muduo_cpp11
