//
// Created by xydai on 2021/8/23.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_LIST_H_
#define DPDK_TCP_INCLUDE_XY_LIST_H_
#include <stddef.h>

typedef struct xy_list_head_t {
  struct xy_list_head_t *next;
  struct xy_list_head_t *prev;
} xy_list_node;

#define LIST_HEAD(name) xy_list_node name = {&(name), &(name)}

static inline void xy_list_init(xy_list_node *head) {
  head->prev = head->next = head;
}

static inline void xy_list_add_head(xy_list_node *head,
                                    xy_list_node *new_node) {
  head->next->prev = new_node;
  new_node->next = head->next;
  new_node->prev = head;
  head->next = new_node;
}

static inline void xy_list_add_tail(xy_list_node *head,
                                    xy_list_node *new_node) {
  head->prev->next = new_node;
  new_node->prev = head->prev;
  new_node->next = head;
  head->prev = new_node;
}

static inline void xy_list_del(xy_list_node *elem) {
  xy_list_node *prev = elem->prev;
  xy_list_node *next = elem->next;

  prev->next = next;
  next->prev = prev;
}

static inline xy_list_node *xy_list_take_head(xy_list_node *head) {
  xy_list_node *elem = head->next;
  xy_list_del(elem);
  return elem;
}

static inline xy_list_node *xy_list_take_tail(xy_list_node *head) {
  xy_list_node *elem = head->prev;
  xy_list_del(elem);
  return elem;
}

static inline int xy_list_is_empty(xy_list_node *head) {
  return head->next == head;
}
#endif  // DPDK_TCP_INCLUDE_XY_LIST_H_
