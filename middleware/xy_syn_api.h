//
// Created by xydai on 2021/8/31.
//
#pragma once
#ifndef DPDK_TCP_MIDDLEWARE_XY_SYN_API_H_
#define DPDK_TCP_MIDDLEWARE_XY_SYN_API_H_
#include <condition_variable>

#include "xy_mpmc.h"
#include "xy_struct.h"
#include "xy_syn_socks.h"


enum xy_ops_type {
  XY_OPS_CREATE,
  XY_OPS_BIND,
  XY_OPS_LISTEN,
  XY_OPS_ACCEPT,
  XY_OPS_CONNECT,
  XY_OPS_CLOSE,
  XY_OPS_SEND,
  XY_OPS_RECV
};

typedef struct {
  xy_tcp_socket* tcp_sk;
} xy_ops_create;

typedef struct {
  xy_tcp_socket* tcp_sk;
  uint32_t ip;
  uint16_t port;
} xy_ops_bind;

typedef struct {
  xy_tcp_socket* tcp_sk;
} xy_ops_listen;

typedef struct {
  xy_tcp_socket* tcp_sk;
  uint32_t *ip;
  uint16_t *port;
} xy_ops_accept;

typedef struct {
  xy_tcp_socket* tcp_sk;
  uint32_t *ip;
  uint16_t *port;
} xy_ops_connect;

typedef struct {
  xy_tcp_socket* tcp_sk;
} xy_ops_close;

typedef struct {
  xy_tcp_socket* tcp_sk;
  const char *buf;
  size_t len;
  ssize_t ret;
} xy_ops_send;

typedef struct {
  xy_tcp_socket* tcp_sk;
  char *buf;
  size_t len;
  ssize_t ret;
} xy_ops_recv;

typedef struct {
  xy_ops_type type;
  uint8_t done;
  std::mutex mutex;
  std::condition_variable cv;
  union {
    xy_ops_create create_;
    xy_ops_bind bind_;
    xy_ops_listen listen_;
    xy_ops_accept accept_;
    xy_ops_connect connect_;
    xy_ops_close close_;
    xy_ops_send send_;
    xy_ops_recv recv_;
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
