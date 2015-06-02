// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.

// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)
//
// This is a public header file, it must only include public header files.

#ifndef MUDUO_CPP11_NET_ENDIAN_H_
#define MUDUO_CPP11_NET_ENDIAN_H_

#include <stdint.h>

#if defined(__MACH__)
#include <libkern/OSByteOrder.h>

#define htobe16(x) OSSwapHostToBigInt16(x)
#define htole16(x) OSSwapHostToLittleInt16(x)
#define be16toh(x) OSSwapBigToHostInt16(x)
#define le16toh(x) OSSwapLittleToHostInt16(x)
 
#define htobe32(x) OSSwapHostToBigInt32(x)
#define htole32(x) OSSwapHostToLittleInt32(x)
#define be32toh(x) OSSwapBigToHostInt32(x)
#define le32toh(x) OSSwapLittleToHostInt32(x)
 
#define htobe64(x) OSSwapHostToBigInt64(x)
#define htole64(x) OSSwapHostToLittleInt64(x)
#define be64toh(x) OSSwapBigToHostInt64(x)
#define le64toh(x) OSSwapLittleToHostInt64(x)
#elif defined(__ANDROID_API__)
#include <endian.h>
#define be16toh(x) htobe16(x)
#define be32toh(x) htobe32(x)
#define be64toh(x) htobe64(x)
#define le16toh(x) htole16(x)
#define le32toh(x) htole32(x)
#define le64toh(x) htole64(x)
#else
#include <endian.h>
#endif

namespace muduo_cpp11 {
namespace net {
namespace sockets {

// the inline assembler code makes type blur,
// so we disable warnings for a while.
#if defined(__clang__) || __GNUC_MINOR__ >= 6
#pragma GCC diagnostic push
#endif

#if !defined(__MACH__) && !defined(__ANDROID_API__)
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wold-style-cast"
#endif

inline uint64_t HostToNetwork64(uint64_t host64) {
  return htobe64(host64);
}

inline uint32_t HostToNetwork32(uint32_t host32) {
  return htobe32(host32);
}

inline uint16_t HostToNetwork16(uint16_t host16) {
  return htobe16(host16);
}

inline uint64_t NetworkToHost64(uint64_t net64) {
  return be64toh(net64);
}

inline uint32_t NetworkToHost32(uint32_t net32) {
  return be32toh(net32);
}

inline uint16_t NetworkToHost16(uint16_t net16) {
  return be16toh(net16);
}
#if defined(__clang__) || __GNUC_MINOR__ >= 6
#pragma GCC diagnostic pop
#else
#pragma GCC diagnostic warning "-Wconversion"
#pragma GCC diagnostic warning "-Wold-style-cast"
#endif

}  // namespace sockets
}  // namespace net
}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_NET_ENDIAN_H_
