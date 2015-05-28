// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo-cpp11/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_CPP11_NET_HTTP_HTTPRESPONSE_H_
#define MUDUO_CPP11_NET_HTTP_HTTPRESPONSE_H_

#include <map>

namespace muduo_cpp11 {
namespace net {

class Buffer;

class HttpResponse {
 public:
  enum HttpStatusCode {
    kUnknown,
    k200Ok = 200,
    k301MovedPermanently = 301,
    k400BadRequest = 400,
    k404NotFound = 404,
  };

  explicit HttpResponse(bool keepalive)
      : status_code_(kUnknown),
        keepalive_(keepalive) {
  }

  void set_status_code(HttpStatusCode code) {
    status_code_ = code;
  }

  void set_status_message(const std::string& message) {
    status_message_ = message;
  }

  void set_keepalive(bool on) {
    keepalive_ = on;
  }

  bool keepalive() const {
    return keepalive_;
  }

  void set_content_type(const std::string& content_type) {
    AddHeader("Content-Type", content_type);
  }

  // FIXME: replace std::string with StringPiece
  void AddHeader(const std::string& key, const std::string& value) {
    headers_[key] = value;
  }

  void set_body(const std::string& body) {
    body_ = body;
  }

  void AppendToBuffer(Buffer* output) const;

 private:
  std::map<std::string, std::string> headers_;
  HttpStatusCode status_code_;
  // FIXME: add http version
  std::string status_message_;
  bool keepalive_;
  std::string body_;
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_HTTP_HTTPRESPONSE_H_
