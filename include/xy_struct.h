//
// Created by xydai on 2021/8/20.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_STRUCT_H_
#define DPDK_TCP_INCLUDE_XY_STRUCT_H_
#include "xy_list.h"
#include "xy_ring.h"

struct tcb {
  /// oldest unacknowledged sequence number
  uint32_t snd_una;
  ///  next sequence number to be sent
  uint32_t snd_nxt;
  uint16_t snd_wnd;
  uint32_t snd_up;
  uint32_t snd_wl1;
  uint32_t snd_wl2;
  uint32_t iss;
  /// next sequence number expected on an incoming segments, and is the left or
  /// lower edge of the receive window
  uint32_t rcv_nxt;
  uint32_t rcv_wnd;
  uint32_t rcv_up;
  uint32_t irs;

  xy_ring_buffer recv_buf;
  xy_ring_buffer send_buf;
};

enum tcp_states {
  /// represents waiting for a connection request from any remote TCP and port.
  TCP_LISTEN,
  /// represents waiting for a matching connection request after having sent a
  /// connection request.
  TCP_SYN_SENT,
  /// represents waiting for a confirming connection request acknowledgment
  /// after having both received and sent a connection request.
  TCP_SYN_RECEIVED,
  ///  represents an open connection, data received can be delivered to the
  ///  user. The normal state for the data transfer phase of the connection.
  TCP_ESTABLISHED,
  /// represents waiting for a connection termination request from the remote
  /// TCP, or an acknowledgment of the connection termination request previously
  /// sent.
  TCP_FIN_WAIT_1,
  /// represents waiting for a connection termination request from the remote
  /// TCP.
  TCP_FIN_WAIT_2,
  /// represents no connection state at all.
  TCP_CLOSE,
  /// represents waiting for a connection termination request from the local
  /// user.
  TCP_CLOSE_WAIT,
  /// represents waiting for a connection termination request acknowledgment
  /// from the remote TCP.
  TCP_CLOSING,
  /// represents waiting for an acknowledgment of the  connection termination
  /// request previously sent to the remote TCP (which includes an
  /// acknowledgment of its connection termination request).
  TCP_LAST_ACK,
  /// represents waiting for enough time to pass to be sure the remote TCP
  /// received the acknowledgment of its connection termination request.
  TCP_TIME_WAIT,

};

typedef struct {
  struct rte_ether_addr mac_src;
  struct rte_ether_addr mac_dst;
} xy_eth_socket;

typedef struct {
  uint16_t id;
  rte_be32_t ip_src;
  rte_be32_t ip_dst;
  xy_eth_socket eth_socket;
} xy_ip_socket;

typedef struct {
  list_head list;

  uint32_t id;
  uint8_t passive;
  tcp_states state;
  rte_be16_t port_src;
  rte_be16_t port_dst;
  xy_ip_socket ip_socket;
  struct tcb* tcb;
} xy_tcp_socket;

inline void recv_enqueue(xy_tcp_socket* tcp_sk, struct rte_mbuf* buf) {
  xy_spsc_ring_add(&tcp_sk->tcb->recv_buf, (void*)buf);
}

inline void send_enqueue(xy_tcp_socket* tcp_sk, struct rte_mbuf* buf) {
  xy_spsc_ring_add(&tcp_sk->tcb->send_buf, buf);
}

inline struct rte_mbuf* recv_dequeue(xy_tcp_socket* tcp_sk) {
  return (struct rte_mbuf*) xy_spsc_ring_peek(&tcp_sk->tcb->recv_buf);
}

inline struct rte_mbuf* send_dequeue(xy_tcp_socket* tcp_sk) {
  return (struct rte_mbuf*) xy_spsc_ring_peek(&tcp_sk->tcb->send_buf);
}
#endif  // DPDK_TCP_INCLUDE_XY_STRUCT_H_
