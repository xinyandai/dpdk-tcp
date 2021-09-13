//
// Created by xydai on 2021/9/1.
//
#pragma once
#ifndef DPDK_TCP_NETWORK_XY_TCP_STATE_CLOSE_H_
#define DPDK_TCP_NETWORK_XY_TCP_STATE_CLOSE_H_
#include "xy_api.h"

static inline int state_tcp_close(xy_tcp_socket *tcp_sk, struct rte_mbuf *m_buf,
                                  struct rte_tcp_hdr *tcp_h,
                                  struct rte_ipv4_hdr *iph,
                                  struct rte_ether_hdr *eh) {
  const uint8_t tcp_flags = tcp_h->tcp_flags;
  if xy_unlikely (tcp_flags & RTE_TCP_RST_FLAG) {
    rte_pktmbuf_free(m_buf);
    return 0;
  }

  if (tcp_flags & RTE_TCP_ACK_FLAG) {
    return tcp_forward(tcp_sk, m_buf, tcp_h, iph, eh, RTE_TCP_RST_FLAG,
                       tcp_h->recv_ack, 0);
  } else {
    return tcp_forward(tcp_sk, m_buf, tcp_h, iph, eh,
                       RTE_TCP_RST_FLAG | RTE_TCP_ACK_FLAG, 0,
                       tcp_h->sent_seq + 1);
  }
}
#endif  // DPDK_TCP_NETWORK_XY_TCP_STATE_CLOSE_H_
