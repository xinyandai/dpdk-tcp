#ifndef __NTY_EPOLL_H__
#define __NTY_EPOLL_H__

#include <stdint.h>
#include "nty_config.h"

typedef enum {

  NTY_EPOLLNONE = 0x0000,
  NTY_EPOLLIN = 0x0001,
  NTY_EPOLLPRI = 0x0002,
  NTY_EPOLLOUT = 0x0004,
  NTY_EPOLLRDNORM = 0x0040,
  NTY_EPOLLRDBAND = 0x0080,
  NTY_EPOLLWRNORM = 0x0100,
  NTY_EPOLLWRBAND = 0x0200,
  NTY_EPOLLMSG = 0x0400,
  NTY_EPOLLERR = 0x0008,
  NTY_EPOLLHUP = 0x0010,
  NTY_EPOLLRDHUP = 0x2000,
  NTY_EPOLLONESHOT = (1 << 30),
  NTY_EPOLLET = (1 << 31)

} nty_epoll_type;

typedef enum {
  NTY_EPOLL_CTL_ADD = 1,
  NTY_EPOLL_CTL_DEL = 2,
  NTY_EPOLL_CTL_MOD = 3,
} nty_epoll_op;

typedef union _nty_epoll_data {
  void *ptr;
  int sockid;
  uint32_t u32;
  uint64_t u64;
} nty_epoll_data;

typedef struct {
  uint32_t events;
  uint64_t data;
} nty_epoll_event;

int nty_epoll_create(int size);
int nty_epoll_ctl(int epid, int op, int sockid, nty_epoll_event *event);
int nty_epoll_wait(int epid, nty_epoll_event *events, int maxevents,
                   int timeout);

#if NTY_ENABLE_EPOLL_RB

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

int nty_epoll_close_socket(int epid);

#endif

#include "nty_buffer.h"
#include "nty_epoll.h"
#include "nty_header.h"
#include "nty_socket.h"

typedef struct _nty_epoll_stat {
  uint64_t calls;
  uint64_t waits;
  uint64_t wakes;

  uint64_t issued;
  uint64_t registered;
  uint64_t invalidated;
  uint64_t handled;
} nty_epoll_stat;

typedef struct _nty_epoll_event_int {
  nty_epoll_event ev;
  int sockid;
} nty_epoll_event_int;

typedef enum {
  USR_EVENT_QUEUE = 0,
  USR_SHADOW_EVENT_QUEUE = 1,
  NTY_EVENT_QUEUE = 2
} nty_event_queue_type;

typedef struct _nty_event_queue {
  nty_epoll_event_int *events;
  int start;
  int end;
  int size;
  int num_events;
} nty_event_queue;

typedef struct _nty_epoll {
  nty_event_queue *usr_queue;
  nty_event_queue *usr_shadow_queue;
  nty_event_queue *queue;

  uint8_t waiting;
  nty_epoll_stat stat;

  pthread_cond_t epoll_cond;
  pthread_mutex_t epoll_lock;
} nty_epoll;

int nty_epoll_add_event(nty_epoll *ep, int queue_type,
                        struct _nty_socket_map *socket, uint32_t event);
int nty_close_epoll_socket(int epid);
int nty_epoll_flush_events(uint32_t cur_ts);

#if NTY_ENABLE_EPOLL_RB

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