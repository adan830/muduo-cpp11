// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo-cpp11/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//

#include "muduo-cpp11/net/http/http_server.h"

#include <string>

#include "muduo-cpp11/base/logging.h"
#include "muduo-cpp11/net/http/http_context.h"
#include "muduo-cpp11/net/http/http_request.h"
#include "muduo-cpp11/net/http/http_response.h"

using std::string;

namespace muduo_cpp11 {
namespace net {
namespace detail {

// FIXME: move to HttpContext class
bool ProcessRequestLine(const char* begin, const char* end, HttpContext* context) {
  bool succeed = false;
  const char* start = begin;
  const char* space = std::find(start, end, ' ');
  HttpRequest& request = context->request();
  if (space != end && request.set_method(start, space)) {
    start = space + 1;
    space = std::find(start, end, ' ');
    if (space != end) {
      const char* question = std::find(start, space, '?');
      if (question != space) {
        request.set_path(start, question);
        request.set_query(question, space);
      } else {
        request.set_path(start, space);
      }
      start = space + 1;
      succeed = (end - start == 8) && std::equal(start, end - 1, "HTTP/1.");
      if (succeed) {
        if (*(end - 1) == '1') {
          request.set_version(HttpRequest::kHttp11);
        } else if (*(end - 1) == '0') {
          request.set_version(HttpRequest::kHttp10);
        } else {
          succeed = false;
        }
      }
    }
  }
  return succeed;
}

// FIXME: move to HttpContext class
// return false if any error
bool ParseRequest(Buffer* buf, HttpContext* context, Timestamp receivetime) {
  bool ok = true;
  bool has_more = true;
  while (has_more) {
    if (context->ExpectRequestLine()) {
      const char* crlf = buf->FindCRLF();
      if (crlf) {
        ok = ProcessRequestLine(buf->Peek(), crlf, context);
        if (ok) {
          context->request().set_receivetime(receivetime);
          buf->RetrieveUntil(crlf + 2);
          context->ReceiveRequestLine();
        } else {
          has_more = false;
        }
      } else {
        has_more = false;
      }
    } else if (context->ExpectHeaders()) {
      const char* crlf = buf->FindCRLF();
      if (crlf) {
        const char* colon = std::find(buf->Peek(), crlf, ':');
        if (colon != crlf) {
          context->request().AddHeader(buf->Peek(), colon, crlf);
        } else {
          // empty line, end of header
          context->ReceiveHeaders();
          has_more = !context->GotAll();
        }
        buf->RetrieveUntil(crlf + 2);
      } else {
        has_more = false;
      }
    } else if (context->ExpectBody()) {
      // FIXME:
    }
  }
  return ok;
}

void DefaultHttpCallback(const HttpRequest&,
                         const HttpServer::RequestDoneCallback& done,
                         HttpResponse* resp) {
  resp->set_status_code(HttpResponse::k404NotFound);
  resp->set_status_message("Not Found");
  resp->set_keepalive(false);
  done(resp);
}

}  // namespace detail

HttpServer::HttpServer(EventLoop* loop,
                       const InetAddress& listen_addr,
                       const string& name,
                       TcpServer::Option option)
    : server_(loop, listen_addr, name, option),
      http_callback_(detail::DefaultHttpCallback) {
  server_.set_connection_callback(std::bind(&HttpServer::OnConnection,
                                            this,
                                            std::placeholders::_1));
  server_.set_message_callback(std::bind(&HttpServer::OnMessage,
                                         this,
                                         std::placeholders::_1,
                                         std::placeholders::_2,
                                         std::placeholders::_3));
}

HttpServer::~HttpServer() {
}

void HttpServer::Start() {
  LOG(INFO) << "HttpServer[" << server_.name() << "] starts listenning on "
            << server_.hostport();
  server_.Start();
}

void HttpServer::OnConnection(const TcpConnectionPtr& conn) {
  if (conn->connected()) {
    conn->set_context(HttpContext(conn->peer_address().ToIp()));
  }
}

void HttpServer::OnMessage(const TcpConnectionPtr& conn,
                           Buffer* buf,
                           Timestamp receive_time) {
  HttpContext* context = boost::any_cast<HttpContext>(conn->mutable_context());

  if (!detail::ParseRequest(buf, context, receive_time)) {
    conn->Send("HTTP/1.1 400 Bad Request\r\n\r\n");
    conn->Shutdown();
  }

  if (context->GotAll()) {
    OnRequest(conn, context->request());
    context->Reset();
  }
}

void HttpServer::OnRequest(const TcpConnectionPtr& conn,
                           const HttpRequest& req) {
  const string& connection = req.GetHeader("Connection");
  bool close = (connection == "close") ||
               (req.version() == HttpRequest::kHttp10 &&
                connection != "Keep-Alive");
  HttpResponse response(!close);
  http_callback_(req,
                 std::bind(&HttpServer::RequestDone, this, conn, std::placeholders::_1),
                 &response);
}

void HttpServer::RequestDone(const TcpConnectionPtr& conn,
                             const HttpResponse* response) {
  Buffer buf;
  response->AppendToBuffer(&buf);
  conn->Send(&buf);
  if (!response->keepalive()) {
    conn->Shutdown();
  }
}

}  // namespace net
}  // namespace muduo_cpp11
