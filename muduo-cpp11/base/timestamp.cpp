#include "muduo-cpp11/base/timestamp.h"

#include <sys/time.h>
#include <stdio.h>

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>

#include <string>
#include <type_traits>

using std::string;

namespace muduo_cpp11 {

static_assert(sizeof(Timestamp) == sizeof(int64_t), "Timestamp type should be int64_t");

Timestamp::Timestamp(int64_t microseconds)
    : microseconds_since_epoch_(microseconds) {
}

string Timestamp::ToString() const {
  char buf[32] = {0};
  int64_t seconds = microseconds_since_epoch_ / kMicroSecondsPerSecond;
  int64_t microseconds = microseconds_since_epoch_ % kMicroSecondsPerSecond;
  snprintf(buf, sizeof(buf) - 1, "%" PRId64 ".%06" PRId64 "", seconds, microseconds);
  return buf;
}

string Timestamp::ToFormattedString(bool show_microseconds) const {
  char buf[32] = {0};
  time_t seconds = static_cast<time_t>(microseconds_since_epoch_ / kMicroSecondsPerSecond);
  struct tm tm_time;
  gmtime_r(&seconds, &tm_time);

  if (show_microseconds) {
    int microseconds = static_cast<int>(microseconds_since_epoch_ % kMicroSecondsPerSecond);
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d.%06d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec,
             microseconds);
  } else {
    snprintf(buf, sizeof(buf), "%4d%02d%02d %02d:%02d:%02d",
             tm_time.tm_year + 1900, tm_time.tm_mon + 1, tm_time.tm_mday,
             tm_time.tm_hour, tm_time.tm_min, tm_time.tm_sec);
  }
  return buf;
}

Timestamp Timestamp::Now() {
  struct timeval tv;
  gettimeofday(&tv, NULL);
  int64_t seconds = tv.tv_sec;
  return Timestamp(seconds * kMicroSecondsPerSecond + tv.tv_usec);
}

Timestamp Timestamp::Invalid() {
  return Timestamp();
}

}  // namespace muduo_cpp11
