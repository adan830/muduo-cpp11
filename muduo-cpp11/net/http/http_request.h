// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo-cpp11/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_CPP11_NET_HTTP_HTTPREQUEST_H_
#define MUDUO_CPP11_NET_HTTP_HTTPREQUEST_H_

#include <assert.h>
#include <stdio.h>

#include <string>
#include <map>

#include "muduo-cpp11/base/timestamp.h"

namespace muduo_cpp11 {
namespace net {

class HttpRequest {
 public:
  enum Method {
    kInvalid, kGet, kPost, kHead, kPut, kDelete
  };

  enum Version {
    kUnknown, kHttp10, kHttp11
  };

  HttpRequest()
      : method_(kInvalid),
        version_(kUnknown) {
  }

  void set_version(Version v) {
    version_ = v;
  }

  Version version() const {
    return version_;
  }

  bool set_method(const char* start, const char* end) {
    assert(method_ == kInvalid);
    std::string m(start, end);
    if (m == "GET") {
      method_ = kGet;
    } else if (m == "POST") {
      method_ = kPost;
    } else if (m == "HEAD") {
      method_ = kHead;
    } else if (m == "PUT") {
      method_ = kPut;
    } else if (m == "DELETE") {
      method_ = kDelete;
    } else {
      method_ = kInvalid;
    }
    return method_ != kInvalid;
  }

  Method method() const {
    return method_;
  }

  const char* method_string() const {
    const char* result = "UNKNOWN";
    switch(method_) {
      case kGet:
        result = "GET";
        break;
      case kPost:
        result = "POST";
        break;
      case kHead:
        result = "HEAD";
        break;
      case kPut:
        result = "PUT";
        break;
      case kDelete:
        result = "DELETE";
        break;
      default:
        break;
    }
    return result;
  }

  void set_path(const char* start, const char* end) {
    path_.assign(start, end);
  }

  const std::string& path() const {
    return path_;
  }

  void set_query(const char* start, const char* end) {
    query_.assign(start, end);
  }

  const std::string& query() const {
    return query_;
  }

  void set_remote_addr(const std::string& remote_addr) {
    remote_addr_ = remote_addr;
  }

  const std::string& remote_addr() const {
    return remote_addr_;
  }

  void set_receivetime(Timestamp t) {
    receivetime_ = t;
  }

  Timestamp receivetime() const {
    return receivetime_;
  }

  void AddHeader(const char* start, const char* colon, const char* end) {
    std::string field(start, colon);
    ++colon;
    while (colon < end && isspace(*colon)) {
      ++colon;
    }
    std::string value(colon, end);
    while (!value.empty() && isspace(value[value.size()-1])) {
      value.resize(value.size()-1);
    }
    headers_[field] = value;
  }

  std::string GetHeader(const std::string& field) const {
    std::string result;
    std::map<std::string, std::string>::const_iterator it = headers_.find(field);
    if (it != headers_.end()) {
      result = it->second;
    }
    return result;
  }

  const std::map<std::string, std::string>& headers() const {
    return headers_;
  }

  void swap(HttpRequest& that) {
    std::swap(method_, that.method_);
    path_.swap(that.path_);
    query_.swap(that.query_);
    remote_addr_.swap(that.remote_addr_);
    receivetime_.swap(that.receivetime_);
    headers_.swap(that.headers_);
  }

 private:
  Method method_;
  Version version_;
  std::string path_;
  std::string query_;
  std::string remote_addr_;
  Timestamp receivetime_;
  std::map<std::string, std::string> headers_;
};

}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_HTTP_HTTPREQUEST_H_
