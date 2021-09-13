//
// Created by xydai on 2021/8/31.
//
#pragma once
#ifndef DPDK_TCP_MIDDLEWARE_XY_MPMC_H_
#define DPDK_TCP_MIDDLEWARE_XY_MPMC_H_
#include <mutex>
#include "xy_macros.h"

typedef struct mpmc_list_node_t {
  struct mpmc_list_node_t* next;
  struct mpmc_list_node_t* prev;
  alignas(XY_CACHE_LINE_SIZE) std::mutex mtx;
} mpmc_list_node;

typedef struct {
  mpmc_list_node head;
  mpmc_list_node tail;
} mpmc_list;

static inline int mpmc_list_add_head(mpmc_list_node* head,
                                     mpmc_list_node* node) {
  if (!head || !node) return -1;
  mpmc_list_node* next = head->next;
  std::lock_guard<std::mutex> head_lock(head->mtx);
  std::lock_guard<std::mutex> next_lock(next->mtx);
  head->next = node;
  node->next = next;
  next->prev = node;
  node->prev = head;
  return 0;
}

static inline int mpmc_list_add_tail(mpmc_list_node* tail,
                                     mpmc_list_node* node) {
  if (!tail || !node) return -1;
  mpmc_list_node* prev = tail->prev;
  std::lock_guard<std::mutex> prev_lock(prev->mtx);
  std::lock_guard<std::mutex> tail_lock(tail->mtx);

  prev->next = node;
  node->next = tail;
  tail->prev = node;
  node->prev = prev;
  return 0;
}

static inline mpmc_list_node* mpmc_list_del_head(mpmc_list_node* head,
                                                 mpmc_list_node* tail) {
  std::lock_guard<std::mutex> head_lock(head->mtx);
  /// can not add head since the head is locked,
  /// first is always a valid  pointer since no one can delete it
  mpmc_list_node* first = head->next;
  /// if first is tail: return NULL
  if (first == tail) {
    return NULL;
  }
  /// else : first->next exists
  std::lock_guard<std::mutex> first_lock(first->mtx);
  /// second is always a valid  pointer since no one can delete it
  mpmc_list_node* second = first->next;
  std::lock_guard<std::mutex> second_lock(second->mtx);

  head->next = second;
  second->prev = head;
  return first;
}
#endif  // DPDK_TCP_MIDDLEWARE_XY_MPMC_H_
