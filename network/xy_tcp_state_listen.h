//
// Created by xydai on 2021/9/1.
//
#pragma once
#ifndef DPDK_TCP_NETWORK_XY_TCP_STATE_LISTEN_H_
#define DPDK_TCP_NETWORK_XY_TCP_STATE_LISTEN_H_
#include "cstring"
#include "xy_api.h"

static inline int state_tcp_listen(xy_tcp_socket *listener,
                                   struct rte_mbuf *m_buf,
                                   struct rte_tcp_hdr *tcp_h,
                                   struct rte_ipv4_hdr *iph,
                                   struct rte_ether_hdr *eh) {
  const uint8_t tcp_flags = tcp_h->tcp_flags;

  if xy_unlikely (tcp_flags & RTE_TCP_RST_FLAG) {  /// first check for an RST
    rte_pktmbuf_free(m_buf);
    return 0;
  }

  if (tcp_flags & RTE_TCP_ACK_FLAG) {  /// second check for an ACK
    // send <SEQ=SEG.ACK><CTL=RST> for ACK
    return tcp_forward(listener, m_buf, tcp_h, iph, eh, RTE_TCP_RST_FLAG,
                       tcp_h->recv_ack, 0);
  }

  if (tcp_flags & RTE_TCP_SYN_FLAG) {  /// third check for a SYN
    // TODO check security
    xy_tcp_socket *tcp_sk = allocate_tcp_socket();
    rte_memcpy(tcp_sk, listener, sizeof(xy_tcp_socket));

    tcp_sk->id = tcp_socket_id();
    tcp_sk->ref_cnt = 1;
    tcp_sk->passive = 1;
    tcp_sk->state = TCP_SYN_RECEIVED;
    *tcp_sk = {.id = tcp_socket_id(),
               .ref_cnt = 1,
               .passive = 1,
               .state = TCP_SYN_RECEIVED,
               .port_src = tcp_h->dst_port,
               .port_dst = tcp_h->src_port,
               .ip_socket = {.id = 0,
                             .ip_src = iph->dst_addr,
                             .ip_dst = iph->src_addr,
                             .eth_socket = {.mac_src = eh->d_addr,
                                            .mac_dst = eh->s_addr}}};

    tcp_sk->tcb = allocate_tcb();
    std::memset(tcp_sk->tcb, 0, sizeof(struct tcb));

    uint32_t iss = random_generate_iss();
    *(tcp_sk->tcb) = {
        .snd_una = iss,
        .snd_nxt = iss + 1,
        .snd_wnd = tcp_h->rx_win,
        .snd_up = 0,  // TODO
        .iss = iss,
        .rcv_nxt = tcp_h->sent_seq + 1,
        .rcv_wnd = XY_RCV_WND_SIZE,
        .rcv_up = 0,  // TODO
        .irs = tcp_h->sent_seq,
    };

    syn_recv_tcp_sock_enqueue(tcp_sk);

    uint8_t sent_tcp_flags = RTE_TCP_SYN_FLAG | RTE_TCP_ACK_FLAG;
    return tcp_forward(tcp_sk, m_buf, tcp_h, iph, eh, sent_tcp_flags,
                       tcp_sk->tcb->iss, tcp_sk->tcb->rcv_nxt);
  }

  rte_pktmbuf_free(m_buf);
  return 0;
}
#endif  // DPDK_TCP_NETWORK_XY_TCP_STATE_LISTEN_H_
