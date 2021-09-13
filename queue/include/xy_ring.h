//
// Created by xydai on 2021/8/27.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_RING_H_
#define DPDK_TCP_INCLUDE_XY_RING_H_

#include <atomic>
#include "include/xy_macros.h"

#define XY_RING_SIZE (1 << 16)

typedef struct {
  alignas(XY_CACHE_LINE_SIZE) std::atomic<size_t> head_;
  alignas(XY_CACHE_LINE_SIZE) std::atomic<size_t> tail_;
  void* buffer[XY_RING_SIZE];
} xy_ring_buffer;


///
inline int xy_ring_empty(xy_ring_buffer* ring) {
  size_t head = ring->head_.load(std::memory_order_relaxed);
  size_t tail = ring->tail_.load(std::memory_order_relaxed);
  return head == tail;
}

inline void* xy_spsc_ring_peek(xy_ring_buffer* ring) {
  size_t head = ring->head_.load(std::memory_order_relaxed);
  size_t tail = ring->tail_.load(std::memory_order_relaxed);
  if (head == tail) {
    return NULL;
  }
  return ring->buffer[head];
}

/// producer's function
inline int xy_spsc_ring_add(xy_ring_buffer* ring, void* ptr) noexcept {
  size_t head = ring->head_.load(std::memory_order_relaxed);
  size_t tail = ring->tail_.load(std::memory_order_relaxed);
  size_t next = tail + 1;

  if xy_unlikely (next == XY_RING_SIZE) {
    next = 0;
  }

  if (next == head) {
    return -1;
  }

  ring->buffer[tail] = ptr;
  ring->tail_.store(next, std::memory_order_relaxed);
  return 0;
}

/// consumer's function
inline size_t xy_spsc_ring_del(xy_ring_buffer* ring, size_t n) {
  size_t head = ring->head_.load(std::memory_order_relaxed);
  size_t tail = ring->tail_.load(std::memory_order_relaxed);

  if (head <= tail) {  // remove contiguous (min(length, n)) elements
    head = xy_min(tail, head + n);
  } else {
    head += n;
    if xy_unlikely (head >= XY_RING_SIZE) {
      head -= XY_RING_SIZE;
      if (head > tail) {
        n -= head - tail;  // exceed the boundary
        head = tail;
      }
    }
  }

  ring->head_.store(head, std::memory_order_relaxed);
  return n;
}

#endif  // DPDK_TCP_INCLUDE_XY_RING_H_
