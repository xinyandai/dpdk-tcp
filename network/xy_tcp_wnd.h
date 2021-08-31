//
// Created by xydai on 2021/8/30.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_TCP_WND_H_
#define DPDK_TCP_INCLUDE_XY_TCP_WND_H_

#define XY_RCV_WND_SIZE (1 << 12)
typedef struct {
  uint16_t head_;
  struct rte_mbuf* buffer[XY_RCV_WND_SIZE];
} xy_tcp_rcv_wnd;

#define XY_SND_WND_SIZE (1 << 12)
typedef struct {
  uint16_t head_;
  uint16_t tail_;
  struct rte_mbuf* buffer[XY_SND_WND_SIZE];
} xy_tcp_snd_wnd;

/// for receive window
static inline int xy_tcp_rcv_wnd_try_put(xy_tcp_rcv_wnd* window,
                                         uint32_t head_seq, uint32_t seq,
                                         struct rte_mbuf* mbuf) {
  uint32_t offset = seq - head_seq;
  if (offset < 0 || offset > XY_RCV_WND_SIZE) {
    return 0;
  }
  size_t slot = window->head_ + offset;
  if xy_unlikely (slot >= XY_RCV_WND_SIZE) {
    slot -= XY_RCV_WND_SIZE;
  }
  window->buffer[slot] = mbuf;
  return 1;
}

static inline uint32_t xy_tcp_rcv_wnd_collect(xy_tcp_rcv_wnd* window,
                                              xy_mbuf_list* mbuf_list) {
  uint32_t count = 0;
  for (int i = window->head_; i < XY_RCV_WND_SIZE; ++i, ++count) {
    if xy_unlikely (window->buffer[i] == NULL) {
      window->head_ = i;
      return count;
    }
    xy_mbuf_list_add_tail(mbuf_list, window->buffer[i]);
    window->buffer[i] = NULL;
  }

  for (int i = 0; i < window->head_; ++i, ++count) {
    if xy_unlikely (window->buffer[i] == NULL) {
      window->head_ = i;
      return count;
    }
    xy_mbuf_list_add_tail(mbuf_list, window->buffer[i]);
    window->buffer[i] = NULL;
  }
  /// window->head_ = window->head_;
  return count;
}

/// for send window
static inline uint32_t xy_tcp_snd_wnd_erase(xy_tcp_snd_wnd* window, int n) {
  uint32_t head = window->head_ + n;
  window->head_ = head < XY_SND_WND_SIZE ? head : head - XY_SND_WND_SIZE;
  return n;
}

static inline uint16_t xy_tcp_snd_wnd_size(xy_tcp_snd_wnd* window) {
  return window->head_ > window->tail_
             ? XY_SND_WND_SIZE + window->tail_ - window->head_
             : window->tail_ - window->head_;
}

static inline int xy_tcp_snd_wnd_add(xy_tcp_snd_wnd* window, struct rte_mbuf* mbuf) {
  window->buffer[window->tail_++] = mbuf;
  if (window->tail_ == XY_SND_WND_SIZE) {
    window->tail_ = 0;
  }
  return 0;
}

#endif  // DPDK_TCP_INCLUDE_XY_TCP_WND_H_
