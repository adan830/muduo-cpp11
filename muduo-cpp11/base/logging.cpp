#include "muduo-cpp11/base/logging.h"

#include <unistd.h>
#include <sys/syscall.h>

#include <errno.h>
#include <string.h>

namespace muduo_cpp11 {

__thread char t_errnobuf[512];
const char* strerror_tl(int saved_errno) {
  return strerror_r(saved_errno, t_errnobuf, sizeof t_errnobuf);
}

pid_t gettid() {
#if defined(__ANDROID_API__)
// gettid() is defined in <unistd.h> in Bionic.
  return ::gettid();
#elif defined(__MACH__)
  return pthread_mach_thread_np(pthread_self());
#else
  return static_cast<pid_t>(::syscall(SYS_gettid));
#endif
}

}  // namespace muduo_cpp11
