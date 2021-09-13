//
// Created by xydai on 2021/8/31.
//
#pragma once
#ifndef DPDK_TCP_MIDDLEWARE_XY_SYN_API_H_
#define DPDK_TCP_MIDDLEWARE_XY_SYN_API_H_
#include <condition_variable>

#include "xy_struct.h"
#include "xy_mpmc.h"
#include "xy_syn_socks.h"

enum xy_ops_type {
  XY_TCP_OPS_CREATE,
  XY_TCP_OPS_BIND,
  XY_TCP_OPS_LISTEN,
  XY_TCP_OPS_ACCEPT,
  XY_TCP_OPS_CONNECT,
  XY_TCP_OPS_CLOSE,
  XY_TCP_OPS_SEND,
  XY_TCP_OPS_RECV,

  XY_EPOLL_OPS_CREAT,
};

typedef struct {
  int sock_id;
} xy_ops_create;

typedef struct {
  int sock_id;
  uint32_t ip;
  uint16_t port;
} xy_ops_bind;

typedef struct {
  int sock_id;
} xy_ops_listen;

typedef struct {
  int sock_id;
  uint32_t *ip;
  uint16_t *port;
} xy_ops_accept;

typedef struct {
  int sock_id;
  uint32_t *ip;
  uint16_t *port;
} xy_ops_connect;

typedef struct {
  int sock_id;
} xy_ops_close;

typedef struct {
  int sock_id;
  const char *buf;
  size_t len;
  ssize_t ret;
} xy_ops_send;

typedef struct {
  int sock_id;
  char *buf;
  size_t len;
  ssize_t ret;
} xy_ops_recv;

typedef struct {
  int size;
  int sock_id;
} xy_epoll_ops_create;

typedef struct {
  xy_ops_type type;
  uint8_t done;
  std::mutex mutex;
  std::condition_variable cv;
  int err_no;

  union {
    xy_ops_create create_;
    xy_ops_bind bind_;
    xy_ops_listen listen_;
    xy_ops_accept accept_;
    xy_ops_connect connect_;
    xy_ops_close close_;
    xy_ops_send send_;
    xy_ops_recv recv_;
    xy_epoll_ops_create epoll_create_;
  };
} xy_socket_ops;


/**
 * \brief Called by dpdk thread to handle ops passed from other thread.
 */
void xy_syn_event_handle();

/**
 * \brief Called by user thread to pass ops to dpdk thread.
 * \param ops
 */
void xy_syn_event_enqueue(xy_socket_ops* ops);

#endif  // DPDK_TCP_MIDDLEWARE_XY_SYN_API_H_
