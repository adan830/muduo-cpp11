// Copyright 2010, Shuo Chen.  All rights reserved.
// http://code.google.com/p/muduo/
//
// Use of this source code is governed by a BSD-style license
// that can be found in the License file.
//
// Author: Shuo Chen (chenshuo at chenshuo dot com)
// Updated by Yifan Fan (yifan.fan.1983@gmail.com)

#ifndef MUDUO_CPP11_BASE_WEAK_CALLBACK_H_
#define MUDUO_CPP11_BASE_WEAK_CALLBACK_H_

#include <functional>
#include <memory>

namespace muduo_cpp11 {

// A barely usable WeakCallback
// This is the C++98/03 version, which doesn't support arguments.

template<typename CLASS>
class WeakCallback {
 public:
  WeakCallback(const std::weak_ptr<CLASS>& object,
               const std::function<void(CLASS*)>& function)
      : object_(object), function_(function) {
  }

  // Default dtor, copy ctor and assignment are okay

  void operator()() const {
    std::shared_ptr<CLASS> ptr(object_.lock());
    if (ptr) {
      function_(ptr.get());
    }
    // else {
    //   LOG(TRACE) << "expired";
    // }
  }

 private:
  std::weak_ptr<CLASS> object_;
  std::function<void(CLASS*)> function_;
};

template<typename CLASS>
WeakCallback<CLASS> MakeWeakCallback(const std::shared_ptr<CLASS>& object,
                                     void (CLASS::*function)()) {
  return WeakCallback<CLASS>(object, function);
}

template<typename CLASS>
WeakCallback<CLASS> MakeWeakCallback(const std::shared_ptr<CLASS>& object,
                                     void (CLASS::*function)() const) {
  return WeakCallback<CLASS>(object, function);
}

}  // namespace muduo_cpp11

#endif  // MUDUO_CPP11_BASE_WEAK_CALLBACK_H_
