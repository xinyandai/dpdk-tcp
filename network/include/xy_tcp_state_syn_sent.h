//
// Created by xydai on 2021/9/1.
//
#pragma once
#ifndef DPDK_TCP_NETWORK_XY_TCP_STATE_SYN_SENT_H_
#define DPDK_TCP_NETWORK_XY_TCP_STATE_SYN_SENT_H_
#include "xy_api.h"

static inline int state_tcp_syn_sent(xy_tcp_socket *tcp_sk,
                                     struct rte_mbuf *m_buf,
                                     struct rte_tcp_hdr *tcp_h,
                                     struct rte_ipv4_hdr *iph,
                                     struct rte_ether_hdr *eh) {
  const uint8_t tcp_flags = tcp_h->tcp_flags;
  struct tcb *sock_tcb = get_tcb(tcp_sk);

  /// first check the ACK bit
  if (tcp_flags & RTE_TCP_ACK_FLAG) {
    /// If SND.UNA =< SEG.ACK =< SND.NXT then the ACK is acceptable.
    /// If SEG.ACK =< ISS or SEG.ACK > SND.NXT, send a reset
    /// (unless the RST bit is set, if so drop the segment and return)
    if xy_unlikely (tcp_h->recv_ack <= sock_tcb->iss ||
                    tcp_h->recv_ack > sock_tcb->snd_nxt) {
      if xy_unlikely (tcp_flags & RTE_TCP_RST_FLAG) {
        rte_pktmbuf_free(m_buf);
        return 0;
      } else {  /// send <SEQ=SEG.ACK><CTL=RST>
        return tcp_forward(tcp_sk, m_buf, tcp_h, iph, eh, RTE_TCP_RST_FLAG,
                           tcp_h->recv_ack, 0);
      }
    }
  }

  ///  second check the RST bit
  if xy_unlikely (tcp_flags & RTE_TCP_RST_FLAG) {
    if (tcp_flags & RTE_TCP_ACK_FLAG) {
      /// If the ACK was acceptable then signal the user "error:
      /// connection reset", drop the segment, enter CLOSED state,
      /// delete TCB, and return
      rte_pktmbuf_free(m_buf);
      tcp_sk->state = TCP_CLOSE;
      return 0;
    } else {
      rte_pktmbuf_free(m_buf);
      return 0;
    }
  }

  /// TODO
  /// third check the security and precedence

  /// the ACK was confirmed to be acceptable, or there is no ACK, and it the
  /// segment did not contain a RST.

  /// fourth check the SYN bit,
  if (tcp_flags & RTE_TCP_SYN_FLAG) {
    sock_tcb->rcv_nxt = tcp_h->sent_seq + 1;
    sock_tcb->irs = tcp_h->sent_seq;
    if xy_likely ((tcp_flags & RTE_TCP_ACK_FLAG)) {
      sock_tcb->snd_una = tcp_h->recv_ack;
      /// TODO
      /// any segments on the retransmission queue which are thereby
      /// acknowledged should be removed.
    }

    if (sock_tcb->snd_una > sock_tcb->iss) {
      syn_recv_tcp_sock_dequeue(tcp_sk);
      tcp_sk->state = TCP_ESTABLISHED;
      established_tcp_sock_enqueue(tcp_sk);
      return tcp_forward(tcp_sk, m_buf, tcp_h, iph, eh, RTE_TCP_ACK_FLAG,
                         sock_tcb->snd_nxt, sock_tcb->rcv_nxt);
    } else {
      tcp_sk->state = TCP_SYN_RECEIVED;
      syn_recv_tcp_sock_enqueue(tcp_sk);
      uint8_t sent_tcp_flags = RTE_TCP_ACK_FLAG | RTE_TCP_SYN_FLAG;
      ///  If there are other controls or text in the segment, queue them for
      ///  processing after the ESTABLISHED state has been reached, return.
      return tcp_forward(tcp_sk, m_buf, tcp_h, iph, eh, sent_tcp_flags,
                         sock_tcb->iss, sock_tcb->rcv_nxt);
    }
  }

  /// fifth, if neither of the SYN or RST bits is set
  /// then drop the segment and return.
  rte_pktmbuf_free(m_buf);
  return 0;
}

#endif  // DPDK_TCP_NETWORK_XY_TCP_STATE_SYN_SENT_H_
