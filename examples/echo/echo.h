#ifndef MUDUO_CPP11_EXAMPLES_ECHO_ECHO_H_
#define MUDUO_CPP11_EXAMPLES_ECHO_ECHO_H_

#include "muduo-cpp11/net/tcp_server.h"

// RFC 862
class EchoServer {
 public:
  EchoServer(muduo_cpp11::net::EventLoop* loop,
             const muduo_cpp11::net::InetAddress& listen_addr);

  void Start();  // calls server_.Start();

 private:
  void OnConnection(const muduo_cpp11::net::TcpConnectionPtr& conn);

  void OnMessage(const muduo_cpp11::net::TcpConnectionPtr& conn,
                 muduo_cpp11::net::Buffer* buf,
                 muduo_cpp11::Timestamp time);

  muduo_cpp11::net::TcpServer server_;
};

#endif  // MUDUO_CPP11_EXAMPLES_ECHO_ECHO_H_
