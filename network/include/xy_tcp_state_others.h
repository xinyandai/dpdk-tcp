//
// Created by xydai on 2021/9/1.
//
#pragma once
#ifndef DPDK_TCP_NETWORK_XY_TCP_STATE_OTHERS_H_
#define DPDK_TCP_NETWORK_XY_TCP_STATE_OTHERS_H_
#include "xy_api.h"
#include "xy_epoll.h"

static inline int state_tcp_handle_data(xy_tcp_socket *tcp_sk,
                                        struct rte_mbuf *buf,
                                        struct rte_tcp_hdr *tcp_h) {
  struct tcb *sock_tcb = get_tcb(tcp_sk);
  /// TODO
  epoll_event_callback(tcp_sk->id, XY_EPOLLIN);
  /// enqueue and window management
  tcp_recv_window_update(tcp_sk, buf, tcp_h);

  /// <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>
  struct rte_mbuf *reply_buf = rte_pktmbuf_alloc(buf_pool);
  tcp_send(tcp_sk, reply_buf, RTE_TCP_ACK_FLAG, sock_tcb->snd_nxt,
           sock_tcb->rcv_nxt, 0);
  return 0;
}

static inline void tcp_socket_reclaim(xy_tcp_socket *tcp_sk) {}
/**
 * \brief handle states including:
 *              SYN-RECEIVED STATE
 *              ESTABLISHED STATE
 *              FIN-WAIT-1 STATE
 *              FIN-WAIT-2 STATE
 *              CLOSE-WAIT STATE
 *              CLOSING STATE
 *              LAST-ACK STATE
 *              TIME-WAIT STATE
 * \param tcp_sk
 * \param m_buf
 * \param tcp_h
 * \param iph
 * \param eh
 * \return
 */
static inline int state_tcp_otherwise(xy_tcp_socket *tcp_sk,
                                      struct rte_mbuf *m_buf,
                                      struct rte_tcp_hdr *tcp_h,
                                      struct rte_ipv4_hdr *iph,
                                      struct rte_ether_hdr *eh) {
  struct tcb *sock_tcb = get_tcb(tcp_sk);
  const uint8_t tcp_flags = tcp_h->tcp_flags;

  ///  first check sequence number
  ///  There are four cases for the acceptability test for an incoming
  ///  segment:
  ///
  ///  Segment Receive  Test
  ///  Length  Window
  ///  ------- -------  -------------------------------------------
  ///
  ///   0       0     SEG.SEQ = RCV.NXT
  ///
  ///   0      >0     RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND
  ///
  ///  >0       0     not acceptable
  ///
  ///  >0      >0     RCV.NXT =< SEG.SEQ < RCV.NXT+RCV.WND
  ///              or RCV.NXT =< SEG.SEQ+SEG.LEN-1 < RCV.NXT+RCV.WND
  ///
  ///  If the RCV.WND is zero, no segments will be acceptable, but
  ///  special allowance should be made to accept valid ACKs, URGs and
  ///  RSTs.
  int acceptable;
  if (sock_tcb->rcv_wnd == 0) {
    acceptable =
        tcp_flags & (RTE_TCP_ACK_FLAG | RTE_TCP_URG_FLAG | RTE_TCP_RST_FLAG);
  } else {
    acceptable = tcp_h->sent_seq >= sock_tcb->rcv_nxt ||
                 tcp_h->sent_seq <= sock_tcb->rcv_nxt + sock_tcb->rcv_wnd;
  }

  if xy_unlikely (!acceptable) {
    if (tcp_flags & RTE_TCP_RST_FLAG) {
      rte_pktmbuf_free(m_buf);
      return 0;
    } else {  /// send <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>
      return tcp_forward(tcp_sk, m_buf, tcp_h, iph, eh, RTE_TCP_ACK_FLAG,
                         sock_tcb->snd_nxt, sock_tcb->rcv_nxt);
    }
  }

  /// second check the RST bit
  if xy_unlikely (tcp_flags & RTE_TCP_RST_FLAG) {
    switch (tcp_sk->state) {
      case TCP_SYN_RECEIVED:
        if (tcp_sk->passive) {
          tcp_sk->state = TCP_LISTEN;
          syn_recv_tcp_sock_dequeue(tcp_sk);
        } else {
          tcp_sk->state = TCP_CLOSE;
        }
        tcp_socket_reclaim(tcp_sk);
        rte_pktmbuf_free(m_buf);
        return 0;
      case TCP_ESTABLISHED:
      case TCP_FIN_WAIT_1:
      case TCP_FIN_WAIT_2:
      case TCP_CLOSE_WAIT:
      case TCP_CLOSING:
      case TCP_LAST_ACK:
      case TCP_TIME_WAIT:
      default:
        /// TODO
        /// All segment queues should be flushed.
        tcp_sk->state = TCP_CLOSE;
        tcp_socket_reclaim(tcp_sk);
        rte_pktmbuf_free(m_buf);
        return 0;
    }
  }

  // TODO third check security and precedence

  /// fourth, check the SYN bit,
  /// If the SYN is in the window it is an error
  /// all segment queues should be flushed
  if (tcp_flags & RTE_TCP_SYN_FLAG) {
    tcp_sk->state = TCP_CLOSE;
    tcp_socket_reclaim(tcp_sk);
    rte_pktmbuf_free(m_buf);
    return 0;
  }

  // fifth check the ACK field
  if (!(tcp_flags & RTE_TCP_ACK_FLAG)) {
    rte_pktmbuf_free(m_buf);
    return 0;
  }

  switch (tcp_sk->state) {
    case TCP_SYN_RECEIVED:
      if xy_unlikely (sock_tcb->snd_una > tcp_h->recv_ack ||
                      tcp_h->recv_ack > sock_tcb->snd_nxt) {
        return tcp_forward(tcp_sk, m_buf, tcp_h, iph, eh, RTE_TCP_RST_FLAG,
                           tcp_h->recv_ack, 0);
      } else {
        syn_recv_tcp_sock_dequeue(tcp_sk);
        tcp_sk->state = TCP_ESTABLISHED;
        established_tcp_sock_enqueue(tcp_sk);
      }
    case TCP_ESTABLISHED:
      tcp_ack_window_update(tcp_sk, tcp_h);
      break;
    case TCP_FIN_WAIT_1:
      tcp_ack_window_update(tcp_sk, tcp_h);
      // TODO Way to check whether FIN is acknowledged?
      if (tcp_h->recv_ack == sock_tcb->snd_nxt) {
        tcp_sk->state = TCP_FIN_WAIT_2;
      }
      break;
    case TCP_FIN_WAIT_2:  // TODO??
      tcp_ack_window_update(tcp_sk, tcp_h);
      break;
    case TCP_CLOSE_WAIT:
      tcp_ack_window_update(tcp_sk, tcp_h);
      // TODO Way to check whether FIN is acknowledged?
      if (tcp_h->recv_ack == sock_tcb->snd_nxt) {
        tcp_sk->state = TCP_TIME_WAIT;
      }
      break;
    case TCP_CLOSING:
      tcp_ack_window_update(tcp_sk, tcp_h);
      break;
    case TCP_LAST_ACK:
      if (tcp_h->recv_ack ==
          sock_tcb->snd_nxt) {  // TODO Way to check whether FIN is
        // acknowledged?
        tcp_sk->state = TCP_CLOSE;  // TODO clean it
      }
      break;

    default:
      break;
  }

  /// sixth, check the URG bit
  if xy_unlikely (tcp_flags & RTE_TCP_URG_FLAG) {
    switch (tcp_sk->state) {
      case TCP_ESTABLISHED:
      case TCP_FIN_WAIT_1:
      case TCP_FIN_WAIT_2:
        sock_tcb->rcv_up = xy_max(sock_tcb->rcv_up, tcp_h->tcp_urp);
        break;
      default:
        break;
    }
  }

  /// seventh, process the segment text
  switch (tcp_sk->state) {
    case TCP_ESTABLISHED:
    case TCP_FIN_WAIT_1:
    case TCP_FIN_WAIT_2:
      state_tcp_handle_data(tcp_sk, m_buf, tcp_h);
      break;
    default:
      break;
  }

  /// eighth, check the FIN bit
  if xy_unlikely (tcp_flags & RTE_TCP_FIN_FLAG) {
    switch (tcp_sk->state) {
        /// Do not process the FIN if the state is CLOSED, LISTEN or SYN-SENT
        /// since the SEG.SEQ cannot be validated; drop the segment and return.
      case TCP_CLOSE:
      case TCP_LISTEN:
      case TCP_SYN_SENT:
        rte_pktmbuf_free(m_buf);
        return 0;

      case TCP_ESTABLISHED:
        established_tcp_sock_dequeue(tcp_sk);
        tcp_sk->state = TCP_CLOSE_WAIT;
        break;
      case TCP_SYN_RECEIVED:
        syn_recv_tcp_sock_dequeue(tcp_sk);
        tcp_sk->state = TCP_CLOSE_WAIT;
        break;
      case TCP_FIN_WAIT_1:
        /// TODO
        /// If our FIN has been ACKed, then enter TIME-WAIT, start the time-wait
        /// timer, turn off the other timers; otherwise enter the CLOSING state.
        if (tcp_h->recv_ack == sock_tcb->snd_nxt) {
          tcp_sk->state = TCP_TIME_WAIT;
        } else {
          tcp_sk->state = TCP_CLOSING;
        }
        break;
      case TCP_FIN_WAIT_2:
        /// TODO
        /// Start the time-wait timer, turn off the other timers.
        tcp_sk->state = TCP_TIME_WAIT;
        break;
      case TCP_TIME_WAIT:
        /// TODO
        ///  Restart the time-wait timer,
        break;
      case TCP_CLOSE_WAIT:
      case TCP_CLOSING:
      case TCP_LAST_ACK:
        break;
      default:
        break;
    }
    sock_tcb->rcv_nxt = tcp_h->sent_seq + 1;
    return tcp_forward(tcp_sk, m_buf, tcp_h, iph, eh, RTE_TCP_ACK_FLAG,
                       sock_tcb->snd_nxt, sock_tcb->rcv_nxt);
  }

  return 0;
}

#endif  // DPDK_TCP_NETWORK_XY_TCP_STATE_OTHERS_H_
