#include "muduo-cpp11/base/logging.h"

#include <unistd.h>
#include <sys/syscall.h>

#include <errno.h>
#include <string.h>

#if defined(__MACH__) || defined(__ANDROID_API__)
#define __STDC_FORMAT_MACROS
#include <inttypes.h>

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <fcntl.h>
#endif

#if defined(__ANDROID_API__)
#include <android/log.h>
#endif

using std::string;

namespace muduo_cpp11 {

string strerror_tl(const int saved_errno) {
  char t_errnobuf[512];
  string error;
#if defined(__MACH__) || defined(__ANDROID_API__)
  if (strerror_r(saved_errno, t_errnobuf, sizeof t_errnobuf) != 0) {
    error.assign("Unkown error");
  } else {
    error.assign(t_errnobuf);
  }
#else
  error.assign(strerror_r(saved_errno, t_errnobuf, sizeof t_errnobuf));
#endif
  return error;
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

#if defined(__MACH__) || defined(__ANDROID_API__)
namespace {

const int kLogLineMaxSize = 1024;
const int kLogBuffSize = 64 * 1024;

}  // namespace

Logger::LogLevel g_log_level = Logger::kInfo;

void Logger::set_log_level(const Logger::LogLevel level) {
  g_log_level = level;
}

std::unique_ptr<Logger> Logger::instance_;
std::once_flag Logger::once_flag_;

Logger& Logger::GetInstance() {
  std::call_once(once_flag_, []() {
    instance_.reset(new Logger);
  });
  return *instance_.get();
}

Logger::Logger()
    : log_fd_(STDERR_FILENO),
      log_cached_(false) {
  log_buf_.resize(kLogBuffSize);
  current_buf_pos_ = &log_buf_[0];
}

Logger::~Logger() {
  if (log_fd_ > 0) {
    // Write buffer logs to file before exiting.
    if (current_buf_pos_ - &log_buf_[0]) {
      Fsync(true);
      current_buf_pos_ = &log_buf_[0];
    }

    if (log_fd_ != STDERR_FILENO) {
      close(log_fd_);
      log_fd_ = STDERR_FILENO;
    }
  }
}

bool Logger::set_log_file_path(const std::string& path) {
  if (path.empty()) {
    return true;
  }

  {
    std::lock_guard<std::mutex> lock(log_mutex_);

    int log_fd = -1;
    if ((log_fd = open(path.c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644)) < 0) {
      fprintf(stderr,
              "Open log file \"%s\" to write fail, errno: %d, error info: %s",
              path.c_str(),
              errno,
              strerror_tl(errno).c_str());
      return false;
    }

    if (log_fd_ != STDERR_FILENO) {
      close(log_fd_);
    }

    log_fd_ = log_fd;
    log_file_path_ = path;
  }

  return true;
}

void Logger::Log(const int priority,
                 const char* caption,
                 const bool need_sync,
                 const char* file_name,
                 const char* func_name,
                 const int line_number,
                 const char* format,
                 ...) {
  if (priority >= Logger::kNumLogLevels) {
    return;
  }

  va_list ap;
  va_start(ap, format);
  char text[kLogLineMaxSize];
  int text_len = vsnprintf(text, sizeof(text), format, ap);
  va_end(ap);

  // Log text is too long!
  if ((text_len + 128) > kLogLineMaxSize) {
    fprintf(stderr,
            "file: " __FILE__ ", line: %d, log buff size: %d < log text length: %d ",
            __LINE__,
            kLogLineMaxSize,
            text_len + 128);
    return;
  }

  struct timeval tv;
  gettimeofday(&tv, NULL);
  struct tm local_tm;
  struct tm* tm = localtime_r(&tv.tv_sec, &local_tm);
  int64_t milliseconds = tv.tv_usec / 1000;

  {
    std::lock_guard<std::mutex> lock(log_mutex_);

    // Left space in buffer is not enough, write buffer logs to file.
    if (((current_buf_pos_ - &log_buf_[0]) + text_len + 128) > kLogLineMaxSize) {
      Fsync(false);
    }

    // If file_name has prefix, just remove it.
    const char* p = NULL;
    if ((p = strrchr(file_name, '/')) != NULL) {
      p++;
    } else {
      p = file_name;
    }

    int buff_len = sprintf(current_buf_pos_,
                           "[%04d-%02d-%02d %02d:%02d:%02d:%03" PRId64 "] [tid=%d] [%s::%s:%d] %s - ",
                           tm->tm_year + 1900,
                           tm->tm_mon + 1,
                           tm->tm_mday,
                           tm->tm_hour,
                           tm->tm_min,
                           tm->tm_sec,
                           milliseconds,
                           gettid(),
                           p,
                           func_name,
                           line_number,
                           caption);

    current_buf_pos_ += buff_len;
    memcpy(current_buf_pos_, text, text_len);
    current_buf_pos_ += text_len;
    *current_buf_pos_++ = '\n';

#if defined(__ANDROID_API__)
    // For android debug.
    {
      string android_log(&log_buf_[0], current_buf_pos_ - &log_buf_[0]);
      __android_log_print(ANDROID_LOG_DEBUG, "HDData", "%s", android_log.c_str());
    }
#endif

    if (!log_cached_ || need_sync) {
      Fsync(false);
    }
  }

  if (priority == Logger::kFatal) {
    abort();
  }
}

void Logger::ForceSync() {
  std::lock_guard<std::mutex> lock(log_mutex_);
  Fsync(false);
}

void Logger::Fsync(const bool flush) {
  int result = 0;
  int write_bytes = current_buf_pos_ - &log_buf_[0];

  if (write(log_fd_, &log_buf_[0], write_bytes) != write_bytes) {
      result = (errno != 0) ? errno : EIO;
      fprintf(stderr,
              "file: " __FILE__ ", line: %d, call write fail, errno: %d, error info: %s\n",
              __LINE__,
              result,
              strerror_tl(result).c_str());
  }

  current_buf_pos_ = &log_buf_[0];

  if (flush && log_fd_ != STDERR_FILENO) {
    // The OS will buffer writes to make best use of the limited amount of disk
    // IO available. Writes will typically be flushed within five to thirty
    // seconds; sooner if the programmer (or libraries) calls fdatasync(2) or
    // fsync(2) or sync(2) (which asks for all dirty data to be flushed). Any
    // data in the OS buffers are written to disk (eventually) when the program
    // crashes, lost if the kernel crashes.
    if (fsync(log_fd_) != 0) {
      result = (errno != 0) ? errno : EIO;
      fprintf(stderr,
              "file: " __FILE__ ", line: %d, call fsync fail, errno: %d, error info: %s\n",
              __LINE__,
              result,
              strerror_tl(result).c_str());
    }
  }
}
#endif

}  // namespace muduo_cpp11
