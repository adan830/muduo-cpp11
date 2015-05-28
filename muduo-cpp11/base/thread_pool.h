// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan.1983@gmail.com)

#ifndef MUDUO_CPP11_BASE_THREAD_POOL_H_
#define MUDUO_CPP11_BASE_THREAD_POOL_H_

#include <atomic>
#include <condition_variable>
#include <deque>
#include <memory>
#include <mutex>
#include <functional>
#include <string>
#include <thread>
#include <vector>

#include "muduo-cpp11/base/macros.h"

namespace muduo_cpp11 {

class ThreadPool {
 public:
  typedef std::function<void ()> Task;

  explicit ThreadPool(const std::string& name = "ThreadPool");
  ~ThreadPool();

  // Must be called before Start().
  void set_max_queue_size(const int max_size) {
    max_queue_size_ = max_size;
  }

  void set_thread_init_callback(const Task& cb) {
    thread_init_callback_ = cb;
  }

  void Start(const int num_threads);
  void Stop();

  // Could block if max_queue_size_ > 0
  void Run(const Task& f);

 private:
  bool IsFull() const;
  void RunInThread();
  Task Take();

 private:
  std::mutex mutex_;
  std::condition_variable not_empty_;
  std::condition_variable not_full_;

  std::string name_;
  Task thread_init_callback_;
  std::vector<std::unique_ptr<std::thread>> threads_;
  std::deque<Task> task_queue_;

  size_t max_queue_size_;
  std::atomic<bool> running_;

  DISABLE_COPY_AND_ASSIGN(ThreadPool);
};

}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_BASE_THREAD_POOL_H_
