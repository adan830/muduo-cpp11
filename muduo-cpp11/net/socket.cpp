// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)

#include "muduo-cpp11/net/socket.h"

#include <netinet/in.h>
#include <netinet/tcp.h>
#include <strings.h>  // bzero
#include <stdio.h>  // snprintf

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/net/inet_address.h"
#include "muduo-cpp11/net/sockets_ops.h"


namespace muduo_cpp11 {
namespace net {

Socket::~Socket() {
  sockets::Close(sockfd_);
}

bool Socket::GetTcpInfo(struct tcp_info* tcpi) const {
  socklen_t len = sizeof(*tcpi);
  bzero(tcpi, len);
  return ::getsockopt(sockfd_, SOL_TCP, TCP_INFO, tcpi, &len) == 0;
}

bool Socket::GetTcpInfoString(char* buf, int len) const {
  struct tcp_info tcpi;
  bool ok = GetTcpInfo(&tcpi);
  if (ok) {
    snprintf(buf, len, "unrecovered=%u "
             "rto=%u ato=%u snd_mss=%u rcv_mss=%u "
             "lost=%u retrans=%u rtt=%u rttvar=%u "
             "sshthresh=%u cwnd=%u total_retrans=%u",
             tcpi.tcpi_retransmits,  // Number of unrecovered [RTO] timeouts
             tcpi.tcpi_rto,          // Retransmit timeout in usec
             tcpi.tcpi_ato,          // Predicted tick of soft clock in usec
             tcpi.tcpi_snd_mss,
             tcpi.tcpi_rcv_mss,
             tcpi.tcpi_lost,         // Lost packets
             tcpi.tcpi_retrans,      // Retransmitted packets out
             tcpi.tcpi_rtt,          // Smoothed round trip time in usec
             tcpi.tcpi_rttvar,       // Medium deviation
             tcpi.tcpi_snd_ssthresh,
             tcpi.tcpi_snd_cwnd,
             tcpi.tcpi_total_retrans);  // Total retransmits for entire connection
  }
  return ok;
}

void Socket::BindAddress(const InetAddress& addr) {
  sockets::BindOrDie(sockfd_, addr.GetSockAddrInet());
}

void Socket::Listen() {
  sockets::ListenOrDie(sockfd_);
}

int Socket::Accept(InetAddress* peeraddr) {
  struct sockaddr_in addr;
  bzero(&addr, sizeof addr);
  int connfd = sockets::Accept(sockfd_, &addr);
  if (connfd >= 0) {
    peeraddr->SetSockAddrInet(addr);
  }
  return connfd;
}

void Socket::ShutdownWrite() {
  sockets::ShutdownWrite(sockfd_);
}

void Socket::set_tcpnodelay(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, IPPROTO_TCP, TCP_NODELAY,
               &optval, static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}

void Socket::set_reuseaddr(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEADDR,
               &optval, static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}

void Socket::set_reuseport(bool on) {
#ifdef SO_REUSEPORT
  int optval = on ? 1 : 0;
  int ret = ::setsockopt(sockfd_, SOL_SOCKET, SO_REUSEPORT,
                         &optval, static_cast<socklen_t>(sizeof optval));
  if (ret < 0) {
    LOG(ERROR) << "SO_REUSEPORT failed.";
  }
#else
  if (on) {
    LOG(ERROR) << "SO_REUSEPORT is not supported.";
  }
#endif
}

void Socket::set_keepalive(bool on) {
  int optval = on ? 1 : 0;
  ::setsockopt(sockfd_, SOL_SOCKET, SO_KEEPALIVE,
               &optval, static_cast<socklen_t>(sizeof optval));
  // FIXME CHECK
}

}  // namespace net
}  // namespace muduo_cpp11
