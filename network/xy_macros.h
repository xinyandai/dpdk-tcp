//
// Created by xydai on 2021/8/17.
//

#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_MACROS_H_
#define DPDK_TCP_INCLUDE_XY_MACROS_H_
#include <stdint.h>
#include <assert.h>

#define XY_MAX_TCP (1 << 16)
#define XY_CACHE_LINE_SIZE 64


#define XY_ETH_HDR_LEN sizeof(struct rte_ether_hdr)
#define XY_IP_HDR_LEN sizeof(struct rte_ipv4_hdr)
#define XY_TCP_HDR_LEN sizeof(struct rte_tcp_hdr)
#define XY_TCP_MAX_DATA_LEN RTE_ETHER_MTU - XY_IP_HDR_LEN - XY_TCP_HDR_LEN
#define xy_max(a, b) ( (a) > (b) ? (a) : (b) )
#define xy_min(a, b) ( (a) < (b) ? (a) : (b) )

#if defined(__GNUC__) || defined(__clang__)
#define xy_unlikely(x) (__builtin_expect(!!(x), 0))
#define xy_likely(x)   (__builtin_expect(!!(x), 1))
#else
#define xy_unlikely(x) (x)
#define xy_likely(x) (x)
#endif

#define xy_assert(expr) assert(expr)

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
