//
// Created by xydai on 2021/9/13.
//
#pragma once
#ifndef DPDK_TCP_NETWORK_INCLUDE_XY_MANAGE_H_
#define DPDK_TCP_NETWORK_INCLUDE_XY_MANAGE_H_
#include "xy_epoll.h"
#include "xy_list.h"
#include "xy_macros.h"
#include "xy_struct.h"

enum socket_type {
  XY_TCP_SOCK_UNUSED,
  XY_TCP_SOCK_TCP,
  XY_TCP_SOCK_EPOLL,
};

typedef struct {
  xy_list_node list_node;
  int id;
  int sock_type;
  xy_event_poll* event_poll;
  xy_tcp_socket* tcp_socket;
} xy_socket_map;

int xy_socket_map_init();

xy_tcp_socket* xy_allocate_tcp_socket();
xy_event_poll* xy_allocate_event_poll();

void xy_deallocate_tcp_socket(int sock_id);
void xy_deallocate_socket_map(int sock_id);

xy_tcp_socket* xy_get_tcp_socket_by_id(int sock_id);
xy_event_poll* xy_get_event_poll_by_id(int sock_id);

#endif  // DPDK_TCP_NETWORK_INCLUDE_XY_MANAGE_H_
