//
// Created by xydai on 2021/8/20.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_STRUCT_H_
#define DPDK_TCP_INCLUDE_XY_STRUCT_H_
#include "xy_tcp_wnd.h"
#include "xy_list_mbuf.h"

typedef struct {
  uint16_t left;
  const char *buf;
  struct rte_mbuf* mbuf;
} xy_buf_cursor;

struct tcb {
  /// oldest unacknowledged sequence number
  uint32_t snd_una;
  ///  next sequence number to be sent
  uint32_t snd_nxt;
  uint16_t snd_wnd;
  /// send urgent pointer
  uint32_t snd_up;
  /// segment sequence number used for last window update
  uint32_t snd_wl1;
  /// segment acknowledgment number used for last window update
  uint32_t snd_wl2;
  /// initial send sequence number
  uint32_t iss;
  /// next sequence number expected on an incoming segments, and is the left or
  /// lower edge of the receive window
  uint32_t rcv_nxt;
  uint32_t rcv_wnd;
  /// receive urgent pointer
  uint32_t rcv_up;
  /// initial receive sequence number
  uint32_t irs;
  /// cursor to track rcv_buf_list
  xy_buf_cursor read_cursor;
  /// the unread list of received buffer
  xy_mbuf_list rcv_buf_list;
  /// the unsent list of sent buffer
  xy_mbuf_list snd_buf_list;
  /// the size of receive window is fixed
  xy_tcp_rcv_wnd rcv_wnd_buffer;
  /// the size of send window is dynamic scaled
  xy_tcp_snd_wnd snd_wnd_buffer;
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
  xy_list_node list;

  uint32_t id;
  uint8_t ref_cnt;
  uint8_t passive;
  tcp_states state;
  rte_be16_t port_src;
  rte_be16_t port_dst;
  xy_ip_socket ip_socket;
  struct tcb* tcb;
} xy_tcp_socket;


#endif  // DPDK_TCP_INCLUDE_XY_STRUCT_H_
