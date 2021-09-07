//
// Created by xydai on 2021/8/27.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_TCP_WND_HDL_H_
#define DPDK_TCP_INCLUDE_XY_TCP_WND_HDL_H_
#include "xy_struct.h"

/**
 * \param tcp_sk
 * \return
 */
static inline int tcp_ack_window_update(xy_tcp_socket *tcp_sk,
                                 struct rte_tcp_hdr *tcp_h) {
  struct tcb *tcb = tcp_sk->tcb;

  if xy_likely (tcb->snd_una < tcp_h->recv_ack &&
                tcp_h->recv_ack <= tcb->snd_nxt) {
    /// If SND.UNA < SEG.ACK =< SND.NXT then, set SND.UNA <- SEG.ACK.
    tcb->snd_una = tcp_h->recv_ack;
    /// Any segments on the retransmission queue which are thereby entirely
    /// acknowledged are removed.
    xy_tcp_snd_wnd_erase(&tcb->snd_wnd_buffer, tcp_h->recv_ack - tcb->snd_una);

    if (tcb->snd_wl1 < tcp_h->sent_seq ||
        (tcb->snd_wl1 = tcp_h->sent_seq && tcb->snd_wl2 <= tcp_h->recv_ack)) {
      ///  If (SND.WL1 < SEG.SEQ or (SND.WL1 = SEG.SEQ and SND.WL2 =< SEG.ACK)),
      ///  set SND.WND <- SEG.WND,
      ///  set SND.WL1 <- SEG.SEQ, and
      ///  set SND.WL2 <- SEG.ACK.
      tcb->snd_wnd = rte_be_to_cpu_16(tcp_h->rx_win);  // TODO
      tcb->snd_wl1 = tcp_h->sent_seq;
      tcb->snd_wl2 = tcp_h->recv_ack;
    }
  } else if (tcp_h->recv_ack > tcb->snd_nxt) {
  }
  return 0;
}

/**
 * update the receive window on segment arrives: struct tcb.{rcv_nxt, rcv_wnd}.
 * \param tcp_sk
 * \param buf
 * \param tcp_h
 * \return
 */
static inline int tcp_recv_window_update(xy_tcp_socket *tcp_sk, struct rte_mbuf *buf,
                                  struct rte_tcp_hdr *tcp_h) {
  /// [head_, head_ + wnd_size]
  /// [rcv_nxt, rcv_nxt + wnd_size]: acceptable
  xy_tcp_rcv_wnd *window = &tcp_sk->tcb->rcv_wnd_buffer;
  xy_mbuf_list *buf_list = &tcp_sk->tcb->rcv_buf_list;
  uint32_t sent_seq = tcp_h->sent_seq;
  /// TCP Receive Window enqueue
  if (xy_tcp_rcv_wnd_try_put(window, tcp_sk->tcb->rcv_nxt, sent_seq, buf)) {
    if (sent_seq == tcp_sk->tcb->rcv_nxt) {
      /// Add contiguous Received and Acknowledged Package into list
      uint32_t count = xy_tcp_rcv_wnd_collect(window, buf_list);
      /// update rcv next pointer
      tcp_sk->tcb->rcv_nxt += count;
    }
  }
  return 0;
}

#endif  // DPDK_TCP_INCLUDE_XY_TCP_WND_HDL_H_
