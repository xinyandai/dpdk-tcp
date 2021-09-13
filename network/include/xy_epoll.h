#pragma once
#ifndef __DPDK_TCP_INCLUDE_XY_EPOLL_H__
#define __DPDK_TCP_INCLUDE_XY_EPOLL_H__

#include <stdint.h>
#include "xy_list.h"
#include "xy_rbtree.h"

typedef enum {

  XY_EPOLLNONE = 0x0000,
  XY_EPOLLIN = 0x0001,
  XY_EPOLLPRI = 0x0002,
  XY_EPOLLOUT = 0x0004,
  XY_EPOLLRDNORM = 0x0040,
  XY_EPOLLRDBAND = 0x0080,
  XY_EPOLLWRNORM = 0x0100,
  XY_EPOLLWRBAND = 0x0200,
  XY_EPOLLMSG = 0x0400,
  XY_EPOLLERR = 0x0008,
  XY_EPOLLHUP = 0x0010,
  XY_EPOLLRDHUP = 0x2000,
  XY_EPOLLONESHOT = (1 << 30),
  XY_EPOLLET = (1 << 31)

} xy_epoll_type;

typedef enum {
  XY_EPOLL_CTL_ADD = 1,
  XY_EPOLL_CTL_DEL = 2,
  XY_EPOLL_CTL_MOD = 3,
} xy_epoll_op;

typedef union {
  void *ptr;
  int sockid;
  uint32_t u32;
  uint64_t u64;
} xy_epoll_data;

typedef struct {
  uint32_t events;
  uint64_t data;
} xy_epoll_event;


typedef struct xy_epoll_item_s {
  xy_list_node ready_list_node;
  RB_ENTRY(xy_epoll_item_s) rbt_node;
  int ready;  /// exist in list

  int sock_id;
  xy_epoll_event event;
} xy_epoll_item;

static int sock_cmp(xy_epoll_item *ep1, xy_epoll_item *ep2) {
  if (ep1->sock_id < ep2->sock_id)
    return -1;
  else if (ep1->sock_id == ep2->sock_id)
    return 0;
  return 1;
}

RB_HEAD(_epoll_rb_socket, xy_epoll_item_s);
RB_GENERATE_STATIC(_epoll_rb_socket, xy_epoll_item_s, rbt_node, sock_cmp);

typedef struct _epoll_rb_socket xy_epoll_rb_tree;

struct xy_event_poll {
  int id;
  xy_list_node ready_list;
  int ready_num;
  xy_epoll_rb_tree rb_tree;
  int rb_num;

  int waiting;

  pthread_mutex_t mtx;      // rbtree update
  pthread_spinlock_t lock;  // ready_list update

  pthread_cond_t cond;    // block for event
  pthread_mutex_t cdmtx;  // mutex for cond
};

int xy_epoll_create(int size);
int xy_epoll_ctl(int epoll_id, int op, xy_tcp_socket *sock,
                 xy_epoll_event *event);
int xy_epoll_wait(int epoll_id, xy_epoll_event *events, int max_events,
                  int timeout);

int epoll_event_callback(int sock, uint32_t event);
int xy_epoll_close_socket(int sock);
#endif