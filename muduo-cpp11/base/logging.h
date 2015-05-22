#ifndef MUDUO_CPP11_BASE_LOGGING_H_
#define MUDUO_CPP11_BASE_LOGGING_H_

#include <glog/logging.h>

namespace muduo_cpp11 {

const char* strerror_tl(int saved_errno);
pid_t gettid();

}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_BASE_LOGGING_H_
