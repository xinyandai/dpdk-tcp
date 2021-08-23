//
// Created by xydai on 2021/8/17.
//

#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_MACROS_H_
#define DPDK_TCP_INCLUDE_XY_MACROS_H_
#include <stdint.h>

#define XY_MAX_TCP 65536

#if defined(__GNUC__) || defined(__clang__)
#define xy_unlikely(x) (__builtin_expect(!!(x), 0))
#define xy_likely(x)   (__builtin_expect(!!(x), 1))
#else
#define xy_unlikely(x) (x)
#define xy_likely(x) (x)
#endif

#define xy_return_if(condition, ret) \
  do {                               \
    if (xy_unlikely(condition)) {    \
      return (ret);                  \
    }                                \
  } while (0)

#define xy_rte_exit_if(condition, ...)     \
  do {                                     \
    if (xy_unlikely(condition)) {          \
      rte_exit(EXIT_FAILURE, __VA_ARGS__); \
    }                                      \
  } while (0)

#define xy_warn_if(condition, ...) \
  do {                             \
    if (xy_unlikely(condition)) {  \
      printf(__VA_ARGS__);         \
    }                              \
  } while (0)

#define xy_message_if(condition, ...) \
  do {                                \
    if (xy_unlikely(condition)) {     \
      printf(__VA_ARGS__);            \
    }                                 \
  } while (0)

#define xy_return_errno_if(condition, set_errno, ret, ...) \
  do {                                                  \
    if (xy_unlikely(condition)) {                       \
      errno = set_errno;                                \
      printf(__VA_ARGS__);                              \
      return ret;                                       \
    }                                                   \
  } while (0)

#endif  // DPDK_TCP_INCLUDE_XY_MACROS_H_
