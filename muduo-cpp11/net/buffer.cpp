// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)
//

#include "muduo-cpp11/net/buffer.h"

#include <errno.h>
#include <sys/uio.h>

#include "muduo-cpp11/base/type_conversion.h"
#include "muduo-cpp11/net/sockets_ops.h"

namespace muduo_cpp11 {
namespace net {

const char Buffer::kCRLF[] = "\r\n";

const size_t Buffer::kCheapPrepend;
const size_t Buffer::kInitialSize;

ssize_t Buffer::ReadFd(int fd, int* saved_errno) {
  // saved an ioctl()/FIONREAD call to tell how much to read
  char extrabuf[65536];
  struct iovec vec[2];
  const size_t writable = WritableBytes();
  vec[0].iov_base = begin() + writer_index_;
  vec[0].iov_len = writable;
  vec[1].iov_base = extrabuf;
  vec[1].iov_len = sizeof extrabuf;
  // when there is enough space in this buffer, don't read into extrabuf.
  // when extrabuf is used, we read 128k-1 bytes at most.
  const int iovcnt = (writable < sizeof extrabuf) ? 2 : 1;
  const ssize_t n = sockets::Readv(fd, vec, iovcnt);
  if (n < 0) {
    *saved_errno = errno;
  } else if (implicit_cast<size_t>(n) <= writable) {
    writer_index_ += n;
  } else {
    writer_index_ = buffer_.size();
    Append(extrabuf, n - writable);
  }
  // if (n == writable + sizeof extrabuf) {
  //   goto line_30;
  // }
  return n;
}

}  // namespace net
}  // namespace muduo_cpp11
