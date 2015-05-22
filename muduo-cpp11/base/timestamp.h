#ifndef MUDUO_CPP11_BASE_TIMESTAMP_H_
#define MUDUO_CPP11_BASE_TIMESTAMP_H_

#include <string>

#include "muduo-cpp11/base/macros.h"

namespace muduo_cpp11 {

///
/// Time stamp in UTC, in microseconds resolution.
///
/// This class is immutable.
/// It's recommended to pass it by value, since it's passed in register on x64.
///
class Timestamp {
 public:
  ///
  /// Constucts an invalid Timestamp.
  ///
  Timestamp()
      : microseconds_since_epoch_(0) {
  }

  ///
  /// Constucts a Timestamp at specific time
  ///
  /// @param microseconds_since_epoch
  explicit Timestamp(int64_t microseconds_since_epoch);

  void swap(Timestamp& that) {
    std::swap(microseconds_since_epoch_, that.microseconds_since_epoch_);
  }

  // default copy/assignment/dtor are Okay

  std::string ToString() const;
  std::string ToFormattedString(bool show_microseconds = true) const;

  bool Valid() const {
    return microseconds_since_epoch_ > 0;
  }

  // for internal usage.
  int64_t microseconds_since_epoch() const {
    return microseconds_since_epoch_;
  }

  time_t seconds_since_epoch() const {
    return static_cast<time_t>(
        microseconds_since_epoch_ / kMicroSecondsPerSecond);
  }

  ///
  /// Get time of now.
  ///
  static Timestamp Now();
  static Timestamp Invalid();

  static const int kMicroSecondsPerSecond = 1000 * 1000;
  static const int kMilliSecondsPerSecond = 1000;

 private:
  int64_t microseconds_since_epoch_;
};

inline bool operator<(Timestamp lhs, Timestamp rhs) {
  return lhs.microseconds_since_epoch() < rhs.microseconds_since_epoch();
}

inline bool operator==(Timestamp lhs, Timestamp rhs) {
  return lhs.microseconds_since_epoch() == rhs.microseconds_since_epoch();
}

///
/// Gets time difference of two timestamps, result in microseconds.
///
/// @param high, low
/// @return (high-low) in microseconds
inline int64_t TimeDifferenceMicroSeconds(Timestamp high, Timestamp low) {
  return high.microseconds_since_epoch() - low.microseconds_since_epoch();
}

///
/// Gets time difference of two timestamps, result in milliseconds.
///
/// @param high, low
/// @return (high-low) in milliseconds
inline int64_t TimeDifferenceMilliSeconds(Timestamp high, Timestamp low) {
  int64_t diff = high.microseconds_since_epoch() - low.microseconds_since_epoch();
  return diff / Timestamp::kMilliSecondsPerSecond;
}

///
/// Gets time difference of two timestamps, result in seconds.
///
/// @param high, low
/// @return (high-low) in seconds
/// @c double has 52-bit precision, enough for one-microsecond
/// resolution for next 100 years.
inline double TimeDifference(Timestamp high, Timestamp low) {
  int64_t diff = high.microseconds_since_epoch() - low.microseconds_since_epoch();
  return static_cast<double>(diff) / Timestamp::kMicroSecondsPerSecond;
}

///
/// Add @c seconds to given timestamp.
///
/// @return timestamp+seconds as Timestamp
///
inline Timestamp AddTime(Timestamp timestamp, double seconds) {
  int64_t delta = static_cast<int64_t>(seconds * Timestamp::kMicroSecondsPerSecond);
  return Timestamp(timestamp.microseconds_since_epoch() + delta);
}

}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_BASE_TIMESTAMP_H_
