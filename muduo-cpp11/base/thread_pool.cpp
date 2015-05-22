// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Changed by Yifan Fan (yifan.fan@dian.fm)

#include "muduo-cpp11/base/thread_pool.h"

#include <assert.h>
#include <stdio.h>

#include <functional>
#include <string>

using std::string;

namespace muduo_cpp11 {

ThreadPool::ThreadPool(const string& name)
    : name_(name),
      max_queue_size_(0),
      running_(false) {
}

ThreadPool::~ThreadPool() {
  if (running_) {
    Stop();
  }
}

void ThreadPool::Start(const int num_threads) {
  assert(!running_ && max_queue_size_ > 0);
  if (running_ || max_queue_size_ <= 0) {
    return;
  }

  assert(threads_.empty());

  // NOTE: Threads will check running_, so set running_ to true before starting
  // threads.
  running_ = true;

  threads_.reserve(num_threads);
  for (int i = 0; i < num_threads; ++i) {
    threads_.emplace_back(new std::thread(std::bind(&ThreadPool::RunInThread, this)));
  }

  if (num_threads == 0 && thread_init_callback_) {
    thread_init_callback_();
  }
}

void ThreadPool::Stop() {
  {
    std::unique_lock<std::mutex> lock(mutex_);
    running_ = false;
    not_empty_.notify_all();
  }

  for (size_t i = 0; i < threads_.size(); ++i) {
    threads_[i]->join();
  }
}

void ThreadPool::Run(const Task& task) {
  assert(running);
  if (!running_) {
    return;
  }

  if (threads_.empty()) {
    task();
  } else {
    std::unique_lock<std::mutex> lock(mutex_);
    while (IsFull()) {
      not_full_.wait(lock);
    }

    assert(!IsFull());
    task_queue_.push_back(task);
    not_empty_.notify_one();
  }
}

ThreadPool::Task ThreadPool::Take() {
  std::unique_lock<std::mutex> lock(mutex_);
  while (task_queue_.empty() && running_) {
    not_empty_.wait(lock);
  }

  Task task;
  if (!task_queue_.empty()) {
    task = task_queue_.front();
    task_queue_.pop_front();
    if (max_queue_size_ > 0) {
      not_full_.notify_one();
    }
  }
  return task;
}

bool ThreadPool::IsFull() const {
  return max_queue_size_ > 0 && task_queue_.size() >= max_queue_size_;
}

void ThreadPool::RunInThread() {
  try {
    if (thread_init_callback_) {
      thread_init_callback_();
    }

    while (running_) {
      Task task(Take());
      if (task) {
        task();
      }
    }
  } catch (const std::exception& ex) {
    fprintf(stderr, "exception caught in ThreadPool %s\n", name_.c_str());
    fprintf(stderr, "reason: %s\n", ex.what());
    abort();
  } catch (...) {
    fprintf(stderr, "unknown exception caught in ThreadPool %s\n", name_.c_str());
    throw;  // rethrow
  }
}

}  // namespace muduo_cpp11
