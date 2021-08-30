#pragma once
#ifndef __DPDK_TCP_INCLUDE_XY_EPOLL_H__
#define __DPDK_TCP_INCLUDE_XY_EPOLL_H__

#include <stdint.h>
#include "xy_config.h"

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

typedef union _xy_epoll_data {
  void *ptr;
  int sockid;
  uint32_t u32;
  uint64_t u64;
} xy_epoll_data;

typedef struct {
  uint32_t events;
  uint64_t data;
} xy_epoll_event;

int xy_epoll_create(int size);
int xy_epoll_ctl(int epid, int op, int sockid, xy_epoll_event *event);
int xy_epoll_wait(int epid, xy_epoll_event *events, int maxevents,
                   int timeout);

#if XY_ENABLE_EPOLL_RB

enum EPOLL_EVENTS {
  EPOLLNONE = 0x0000,
  EPOLLIN = 0x0001,
  EPOLLPRI = 0x0002,
  EPOLLOUT = 0x0004,
  EPOLLRDNORM = 0x0040,
  EPOLLRDBAND = 0x0080,
  EPOLLWRNORM = 0x0100,
  EPOLLWRBAND = 0x0200,
  EPOLLMSG = 0x0400,
  EPOLLERR = 0x0008,
  EPOLLHUP = 0x0010,
  EPOLLRDHUP = 0x2000,
  EPOLLONESHOT = (1 << 30),
  EPOLLET = (1 << 31)

};

#define EPOLL_CTL_ADD 1
#define EPOLL_CTL_DEL 2
#define EPOLL_CTL_MOD 3

typedef union epoll_data {
  void *ptr;
  int fd;
  uint32_t u32;
  uint64_t u64;
} epoll_data_t;

struct epoll_event {
  uint32_t events;
  epoll_data_t data;
};

int epoll_create(int size);
int epoll_ctl(int epid, int op, int sockid, struct epoll_event *event);
int epoll_wait(int epid, struct epoll_event *events, int maxevents,
               int timeout);

int xy_epoll_close_socket(int epid);

#endif

#include "xy_buffer.h"
#include "xy_epoll.h"
#include "xy_header.h"
#include "xy_socket.h"

typedef struct _xy_epoll_stat {
  uint64_t calls;
  uint64_t waits;
  uint64_t wakes;

  uint64_t issued;
  uint64_t registered;
  uint64_t invalidated;
  uint64_t handled;
} xy_epoll_stat;

typedef struct _xy_epoll_event_int {
  xy_epoll_event ev;
  int sockid;
} xy_epoll_event_int;

typedef enum {
  USR_EVENT_QUEUE = 0,
  USR_SHADOW_EVENT_QUEUE = 1,
  XY_EVENT_QUEUE = 2
} xy_event_queue_type;

typedef struct _xy_event_queue {
  xy_epoll_event_int *events;
  int start;
  int end;
  int size;
  int num_events;
} xy_event_queue;

typedef struct _xy_epoll {
  xy_event_queue *usr_queue;
  xy_event_queue *usr_shadow_queue;
  xy_event_queue *queue;

  uint8_t waiting;
  xy_epoll_stat stat;

  pthread_cond_t epoll_cond;
  pthread_mutex_t epoll_lock;
} xy_epoll;

int xy_epoll_add_event(xy_epoll *ep, int queue_type,
                        struct _xy_socket_map *socket, uint32_t event);
int xy_close_epoll_socket(int epid);
int xy_epoll_flush_events(uint32_t cur_ts);

#if XY_ENABLE_EPOLL_RB

struct epitem {
  RB_ENTRY(epitem) rbn;
  LIST_ENTRY(epitem) rdlink;
  int rdy;  // exist in list

  int sockfd;
  struct epoll_event event;
};

static int sockfd_cmp(struct epitem *ep1, struct epitem *ep2) {
  if (ep1->sockfd < ep2->sockfd)
    return -1;
  else if (ep1->sockfd == ep2->sockfd)
    return 0;
  return 1;
}

RB_HEAD(_epoll_rb_socket, epitem);
RB_GENERATE_STATIC(_epoll_rb_socket, epitem, rbn, sockfd_cmp);

typedef struct _epoll_rb_socket ep_rb_tree;

struct eventpoll {
  ep_rb_tree rbr;
  int rbcnt;

  LIST_HEAD(, epitem) rdlist;
  int rdnum;

  int waiting;

  pthread_mutex_t mtx;      // rbtree update
  pthread_spinlock_t lock;  // rdlist update

  pthread_cond_t cond;    // block for event
  pthread_mutex_t cdmtx;  // mutex for cond
};

int epoll_event_callback(struct eventpoll *ep, int sockid, uint32_t event);

#endif