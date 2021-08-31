//
// Created by xydai on 2021/8/31.
//
#pragma once
#ifndef DPDK_TCP_MIDDLEWARE_XY_API_ASYN_H_
#define DPDK_TCP_MIDDLEWARE_XY_API_ASYN_H_
#include "xy_mpmc.h"
#include "xy_struct.h"
enum xy_ops_type {
  XY_OPS_CREATE,
  XY_OPS_BIND,
  XY_OPS_LISTEN,
  XY_OPS_ACCEPT,
  XY_OPS_CONNECT,
  XY_OPS_CLOSE,
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
  xy_ops_type type;
  union {
    xy_ops_create create_;
    xy_ops_bind bind_;
    xy_ops_listen listen_;
    xy_ops_accept accept_;
    xy_ops_connect connect_;
    xy_ops_close close_;
  };
} xy_socket_ops;


void xy_asyn_event_handle();
void xy_asyn_event_enqueue(xy_socket_ops* ops);

#endif  // DPDK_TCP_MIDDLEWARE_XY_API_ASYN_H_
