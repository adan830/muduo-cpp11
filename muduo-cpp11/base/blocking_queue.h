// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)

#ifndef MUDUO_CPP11_BASE_BLOCKING_QUEUE_H_
#define MUDUO_CPP11_BASE_BLOCKING_QUEUE_H_

#include <assert.h>

#include <condition_variable>
#include <deque>
#include <mutex>

#include "base/macros.h"

namespace muduo_cpp11 {

template<typename T>
class BlockingQueue {
 public:
  BlockingQueue()
      : queue_() {
  }

  void Put(const T& x) {
    std::unique_lock<std::mutex> lock(mutex_);
    queue_.push_back(x);
    not_empty_.notify_one();  // wait morphing saves us
    // http://www.domaigne.com/blog/computing/condvars-signal-with-mutex-locked-or-not/
  }

  T Take() {
    std::unique_lock<std::mutex> lock(mutex_);
    // always use a while-loop, due to spurious wakeup
    while (queue_.empty()) {
      not_empty_.wait(lock);
    }
    assert(!queue_.empty());
    T front(queue_.front());
    queue_.pop_front();
    return front;
  }

  size_t Size() const {
    std::unique_lock<std::mutex> lock(mutex_);
    return queue_.size();
  }

 private:
  mutable std::mutex mutex_;
  std::condition_variable not_empty_;
  std::deque<T> queue_;

  DISABLE_COPY_AND_ASSIGN(BlockingQueue);
};

}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_BASE_BLOCKING_QUEUE_H_
