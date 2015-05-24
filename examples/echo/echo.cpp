#include "examples/echo/echo.h"

#include <string>

#include "muduo-cpp11/base/logging.h"

using std::placeholders::_1;
using std::placeholders::_2;
using std::placeholders::_3;

EchoServer::EchoServer(muduo_cpp11::net::EventLoop* loop,
                       const muduo_cpp11::net::InetAddress& listen_addr)
  : server_(loop, listen_addr, "EchoServer") {
    server_.set_connection_callback(std::bind(&EchoServer::OnConnection, this, _1));
    server_.set_message_callback(std::bind(&EchoServer::OnMessage, this, _1, _2, _3));
}

void EchoServer::Start() {
  server_.Start();
}

void EchoServer::OnConnection(const muduo_cpp11::net::TcpConnectionPtr& conn) {
  LOG(INFO) << "EchoServer - " << conn->peer_address().ToIpPort() << " -> "
            << conn->local_address().ToIpPort() << " is "
            << (conn->connected() ? "UP" : "DOWN");
}

void EchoServer::OnMessage(const muduo_cpp11::net::TcpConnectionPtr& conn,
                           muduo_cpp11::net::Buffer* buf,
                           muduo_cpp11::Timestamp time) {
  std::string msg(buf->RetrieveAllAsString());
  LOG(INFO) << conn->name() << " echo " << msg.size() << " bytes, "
            << "data received at " << time.ToString();
  conn->Send(msg);
}
