//
// Created by xydai on 2021/8/23.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_LIST_H_
#define DPDK_TCP_INCLUDE_XY_LIST_H_
#include <stddef.h>

typedef struct list_head_t {
  struct list_head_t *next;
  struct list_head_t *prev;
} list_head;

#define LIST_HEAD(name) list_head name = {&(name), &(name)}

static inline void list_init(list_head *head) {
  head->prev = head->next = head;
}

static inline void list_add_head(list_head *head, list_head *new_node) {
  head->next->prev = new_node;
  new_node->next = head->next;
  new_node->prev = head;
  head->next = new_node;
}

static inline void list_add_tail(list_head *head, list_head *new_node) {
  head->prev->next = new_node;
  new_node->prev = head->prev;
  new_node->next = head;
  head->prev = new_node;
}

static inline void list_del(list_head *elem) {
  list_head *prev = elem->prev;
  list_head *next = elem->next;

  prev->next = next;
  next->prev = prev;
}

static inline int list_is_empty(list_head *head) { return head->next == head; }
#endif  // DPDK_TCP_INCLUDE_XY_LIST_H_