//
// Created by xydai on 2021/8/20.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_SOCKS_H_
#define DPDK_TCP_INCLUDE_XY_SOCKS_H_

#include "xy_api.h"

uint32_t tcp_socket_id();
struct tcb *allocate_tcb();
void deallocate_tcb(xy_tcp_socket *tcp_sock);
xy_tcp_socket *allocate_tcp_socket();
void deallocate_tcp_socket(xy_tcp_socket *tcp_sock);

void syn_recv_tcp_sock_enqueue(xy_tcp_socket *tcp_sock);

void syn_recv_tcp_sock_dequeue(xy_tcp_socket *tcp_sock);

void listener_tcp_sock_enqueue(xy_tcp_socket *tcp_sock);

void listener_tcp_sock_dequeue(xy_tcp_socket *tcp_sock);

xy_tcp_socket *listener_tcp_sock_lookup(rte_be32_t dst_ip_be,
                                        rte_be16_t dst_port);

xy_tcp_socket *syn_rcvd_tcp_sock_lookup(rte_be32_t dst_ip_be,
                                        rte_be16_t dst_port_be,
                                        rte_be32_t src_ip_be,
                                        rte_be16_t src_port_be);
#endif  // DPDK_TCP_INCLUDE_XY_SOCKS_H_
