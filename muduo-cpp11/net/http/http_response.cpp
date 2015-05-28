// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)
//

#include "muduo-cpp11/net/http/http_response.h"

#include <stdio.h>

#include <string>

#include "muduo-cpp11/net/buffer.h"

using std::string;

namespace muduo_cpp11 {
namespace net {

void HttpResponse::AppendToBuffer(Buffer* output) const {
  char buf[32];
  snprintf(buf, sizeof buf, "HTTP/1.1 %d ", status_code_);

  output->Append(buf);
  output->Append(status_message_);
  output->Append("\r\n");

  if (!keepalive_) {
    output->Append("Connection: close\r\n");
  } else {
    snprintf(buf, sizeof buf, "Content-Length: %zd\r\n", body_.size());
    output->Append(buf);
    output->Append("Connection: Keep-Alive\r\n");
  }

  for (std::map<string, string>::const_iterator it = headers_.begin();
       it != headers_.end();
       ++it) {
    output->Append(it->first);
    output->Append(": ");
    output->Append(it->second);
    output->Append("\r\n");
  }

  output->Append("\r\n");
  output->Append(body_);
}

}  // namespace net
}  // namespace muduo_cpp11
