// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo-cpp11/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_CPP11_NET_HTTP_HTTPSERVER_H
#define MUDUO_CPP11_NET_HTTP_HTTPSERVER_H

#include <functional>
#include <string>

#include "muduo-cpp11/base/macros.h"
#include "muduo-cpp11/net/tcp_server.h"

namespace muduo_cpp11 {
namespace net {

class HttpRequest;
class HttpResponse;

/// A simple embeddable HTTP server designed for report status of a program.
/// It is not a fully HTTP 1.1 compliant server, but provides minimum features
/// that can communicate with HttpClient and Web browser.
class HttpServer {
 public:
  typedef std::function<void (HttpResponse*)> RequestDoneCallback;
  typedef std::function<void (const HttpRequest&,
                              const RequestDoneCallback&,
                              HttpResponse*)> HttpCallback;

  HttpServer(EventLoop* loop,
             const InetAddress& listen_addr,
             const std::string& name,
             TcpServer::Option option = TcpServer::kNoReusePort);

  ~HttpServer();  // force out-line dtor, for scoped_ptr members.

  EventLoop* GetLoop() const {
    return server_.GetLoop();
  }

  /// Not thread safe, callback be registered before calling start().
  void set_http_callback(const HttpCallback& cb) {
    http_callback_ = cb;
  }

  void set_thread_num(int num_threads) {
    server_.set_thread_num(num_threads);
  }

  void Start();

 private:
  void OnConnection(const TcpConnectionPtr& conn);
  void OnMessage(const TcpConnectionPtr& conn,
                 Buffer* buf,
                 Timestamp receive_time);
  void OnRequest(const TcpConnectionPtr&, const HttpRequest&);
  void RequestDone(const TcpConnectionPtr& conn,
                   const HttpResponse* response);

  TcpServer server_;
  HttpCallback http_callback_;

  DISABLE_COPY_AND_ASSIGN(HttpServer);
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_HTTP_HTTPSERVER_H
