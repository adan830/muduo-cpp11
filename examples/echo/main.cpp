#include "examples/echo/echo.h"

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/net/event_loop.h"

int main() {
  LOG(INFO) << "pid = " << getpid();
  muduo_cpp11::net::EventLoop loop;
  muduo_cpp11::net::InetAddress listen_addr(2007);
  EchoServer server(&loop, listen_addr);
  server.Start();
  loop.Loop();
}

