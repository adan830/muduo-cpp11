// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo-cpp11/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//
// This is an internal header file, you should not include this.

#ifndef MUDUO_CPP11_NET_HTTP_HTTPCONTEXT_H_
#define MUDUO_CPP11_NET_HTTP_HTTPCONTEXT_H_

#include "muduo-cpp11/net/http/http_request.h"

namespace muduo_cpp11 {
namespace net {

class HttpContext {
 public:
  enum HttpRequestParseState {
    kExpectRequestLine,
    kExpectHeaders,
    kExpectBody,
    kGotAll,
  };

  explicit HttpContext(const std::string& remote_addr)
      : state_(kExpectRequestLine) {
    request_.set_remote_addr(remote_addr);
  }

  // default copy-ctor, dtor and assignment are fine

  bool ExpectRequestLine() const {
    return state_ == kExpectRequestLine;
  }

  bool ExpectHeaders() const {
    return state_ == kExpectHeaders;
  }

  bool ExpectBody() const {
    return state_ == kExpectBody;
  }

  bool GotAll() const {
    return state_ == kGotAll;
  }

  void ReceiveRequestLine() {
    state_ = kExpectHeaders;
  }

  void ReceiveHeaders() {  // FIXME
    state_ = kGotAll;
  }

  void Reset() {
    state_ = kExpectRequestLine;
    HttpRequest dummy;
    request_.swap(dummy);
  }

  const HttpRequest& request() const {
    return request_;
  }

  HttpRequest& request() {
    return request_;
  }

 private:
  HttpRequestParseState state_;
  HttpRequest request_;
};

}  // net
}  // muduo-cpp11

#endif  // MUDUO_CPP11_NET_HTTP_HTTPCONTEXT_H_
