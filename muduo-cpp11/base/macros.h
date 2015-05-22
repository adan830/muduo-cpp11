#ifndef MUDUO_CPP11_BASE_MACROS_H_
#define MUDUO_CPP11_BASE_MACROS_H_

#define DISABLE_COPY_AND_ASSIGN(name) \
  name(const name&) = delete; \
  name& operator=(const name&) = delete

#endif  // MUDUO_CPP11_BASE_MACROS_H_
