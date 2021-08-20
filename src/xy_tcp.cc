#include "xy_api.h"

xy_tcp_socket *allocate_tcp_socket() {
  return (xy_tcp_socket *)malloc(sizeof(xy_tcp_socket));
}

int port_register(uint16_t port) { return 0; }

/** check ip address and port
 * \param tcp_h
 * \return 0 if valid
 */
static int check_tcp_hdr(struct rte_tcp_hdr *tcp_h) { return 0; }

int tcp_send(xy_tcp_socket *tcp_sk, struct rte_mbuf *m_buf,
             struct rte_tcp_hdr *tcp_h, struct rte_ipv4_hdr *iph,
             struct rte_ether_hdr *eh, uint8_t tcp_flags,
             rte_be32_t sent_seq, rte_be32_t recv_ack) {
  tcp_h->dst_port = tcp_sk->port_dst;
  tcp_h->src_port = tcp_sk->port_src;
  tcp_h->sent_seq = sent_seq;
  tcp_h->recv_ack = recv_ack;
  tcp_h->data_off = 0;  // TODO
  tcp_h->tcp_flags = tcp_flags;
  tcp_h->rx_win = 0;   // TODO
  tcp_h->cksum = 0;    // TODO
  tcp_h->tcp_urp = 0;  // TODO

  return ip_send(&tcp_sk->ip_socket, m_buf, iph, eh, IPPROTO_TCP);
}

/**
 * @param tcp_sk
 * @param m_buf
 * @param tcp_h
 * @param iph
 * @param eh
 * @return
 */
static inline int state_tcp_close(xy_tcp_socket *tcp_sk,
                                  struct rte_mbuf *m_buf,
                                  struct rte_tcp_hdr *tcp_h,
                                  struct rte_ipv4_hdr *iph,
                                  struct rte_ether_hdr *eh) {
  const uint8_t tcp_flags = tcp_h->tcp_flags;
  if (unlikely(tcp_flags & RTE_TCP_RST_FLAG)) {
    rte_pktmbuf_free(m_buf);
    return 0;
  } else {
    if (tcp_flags & RTE_TCP_ACK_FLAG) {
      return tcp_send(tcp_sk, m_buf, tcp_h, iph, eh, RTE_TCP_RST_FLAG,
                      tcp_h->recv_ack, 0);
    } else {
      return tcp_send(tcp_sk, m_buf, tcp_h, iph, eh,
                      RTE_TCP_RST_FLAG | RTE_TCP_ACK_FLAG, 0,
                      tcp_h->sent_seq + 1);
    }
  }
}

/**
 * @param tcp_sk
 * @param m_buf
 * @param tcp_h
 * @param iph
 * @param eh
 * @return
 */
static inline int state_tcp_listen(xy_tcp_socket *tcp_sk,
                                   struct rte_mbuf *m_buf,
                                   struct rte_tcp_hdr *tcp_h,
                                   struct rte_ipv4_hdr *iph,
                                   struct rte_ether_hdr *eh) {
  const uint8_t tcp_flags = tcp_h->tcp_flags;

  if (tcp_flags & RTE_TCP_RST_FLAG) {  // first check for an RST
    return 0;
  }

  if (tcp_flags & RTE_TCP_ACK_FLAG) {  // second check for an ACK
    // send <SEQ=SEG.ACK><CTL=RST> for ACK
    return tcp_send(tcp_sk, m_buf, tcp_h, iph, eh, RTE_TCP_RST_FLAG,
                    tcp_h->recv_ack, 0);
  }

  if (tcp_flags & RTE_TCP_SYN_FLAG) {  // third check for a SYN
    // TODO check security
    tcp_sk->tcb.rcv_nxt = tcp_h->sent_seq + 1;
    tcp_sk->tcb.irs = tcp_h->sent_seq;

    uint8_t tcp_flags = RTE_TCP_SYN_FLAG | RTE_TCP_ACK_FLAG;
    rte_be32_t sent_seq = tcp_sk->tcb.iss;
    rte_be32_t recv_ack = tcp_sk->tcb.rcv_nxt;

    tcp_sk->tcb.snd_nxt = tcp_sk->tcb.iss + 1;
    tcp_sk->tcb.snd_una = tcp_sk->tcb.iss;

    tcp_sk->state = TCP_SYN_RECEIVED;

    syn_recv_tcp_sock_enqueue(tcp_sk);

    return tcp_send(tcp_sk, m_buf, tcp_h, iph, eh, tcp_flags,
                    sent_seq, recv_ack);
  }

  rte_pktmbuf_free(m_buf);
  return 0;
}

static inline int state_tcp_syn_sent(xy_tcp_socket *tcp_sk,
                                     struct rte_mbuf *m_buf,
                                     struct rte_tcp_hdr *tcp_h,
                                     struct rte_ipv4_hdr *iph,
                                     struct rte_ether_hdr *eh) {
  const uint8_t tcp_flags = tcp_h->tcp_flags;
  if (tcp_flags & RTE_TCP_ACK_FLAG) {  // first check the ACK bit
    // If SND.UNA =< SEG.ACK =< SND.NXT then the  ACK is acceptable.
    // If SEG.ACK =< ISS or SEG.ACK > SND.NXT, send a reset
    // (unless the RST bit is set, if so drop the segment and return)
    if xy_unlikely (tcp_h->recv_ack <= tcp_sk->tcb.iss ||
                    tcp_h->recv_ack > tcp_sk->tcb.snd_nxt) {
      if xy_unlikely (tcp_flags & RTE_TCP_RST_FLAG) {
        rte_pktmbuf_free(m_buf);
        return 0;
      } else {  // send <SEQ=SEG.ACK><CTL=RST>
        return tcp_send(tcp_sk, m_buf, tcp_h, iph, eh,
                        RTE_TCP_RST_FLAG, tcp_h->recv_ack, 0);
      }
    }
  }

  // the ACK was confirmed to be acceptable

  if (tcp_flags & RTE_TCP_RST_FLAG) {
    if (tcp_flags & RTE_TCP_ACK_FLAG) {
      // If the ACK was acceptable then signal the user "error:
      // connection reset", drop the segment, enter CLOSED state,
      // delete TCB, and return
      rte_pktmbuf_free(m_buf);
      tcp_sk->state = TCP_CLOSE;  // TODO delete TCB
      return 0;
    } else {
      rte_pktmbuf_free(m_buf);
      return 0;
    }
  }

  // TODO third check the security and precedence

  // fourth check the SYN bit
  if (tcp_flags & RTE_TCP_SYN_FLAG) {
    tcp_sk->tcb.rcv_nxt = tcp_h->sent_seq + 1;
    tcp_sk->tcb.irs = tcp_h->sent_seq;
    if ((tcp_flags & RTE_TCP_ACK_FLAG)) {
      tcp_sk->tcb.snd_una = tcp_h->recv_ack;
      // TODO any segments on the retransmission queue which
      //        are thereby acknowledged should be removed.
    }

    if (tcp_sk->tcb.snd_una > tcp_sk->tcb.iss) {
      syn_recv_tcp_sock_dequeue(tcp_sk);
      tcp_sk->state = TCP_ESTABLISHED;
      established_tcp_sock_enqueue(tcp_sk);
      return tcp_send(tcp_sk, m_buf, tcp_h, iph, eh, RTE_TCP_ACK_FLAG,
                      tcp_sk->tcb.snd_nxt, tcp_sk->tcb.rcv_nxt);
    }

    tcp_sk->state = TCP_SYN_RECEIVED;
    syn_recv_tcp_sock_enqueue(tcp_sk);
    return tcp_send(tcp_sk, m_buf, tcp_h, iph, eh,
                    RTE_TCP_ACK_FLAG | RTE_TCP_SYN_FLAG,
                    tcp_sk->tcb.iss, tcp_sk->tcb.rcv_nxt);
  }

  rte_pktmbuf_free(m_buf);
  return 0;
}

static inline int state_tcp_handle_ack(xy_tcp_socket *tcp_sk,
                                       struct rte_tcp_hdr *tcp_h) {
  if xy_likely (tcp_sk->tcb.snd_una < tcp_h->recv_ack &&
                tcp_h->recv_ack <= tcp_sk->tcb.snd_nxt) {
    tcp_sk->tcb.snd_una = tcp_h->recv_ack;
    // TODO Remove acknowledged segments on the retransmission queue
    if (tcp_sk->tcb.snd_wl1 < tcp_h->sent_seq ||
        (tcp_sk->tcb.snd_wl1 =
             tcp_h->sent_seq &&
             tcp_sk->tcb.snd_wl2 <= tcp_h->recv_ack)) {
      tcp_sk->tcb.snd_wnd = rte_be_to_cpu_16(tcp_h->rx_win);  // TODO
      tcp_sk->tcb.snd_wl1 = tcp_h->sent_seq;
      tcp_sk->tcb.snd_wl2 = tcp_h->recv_ack;
    }
  }
  return 0;
}

/**
 * @param tcp_sk
 * @param m_buf
 * @param tcp_h
 * @param iph
 * @param eh
 * @return
 */
static inline int state_tcp_otherwise(xy_tcp_socket *tcp_sk,
                                      struct rte_mbuf *m_buf,
                                      struct rte_tcp_hdr *tcp_h,
                                      struct rte_ipv4_hdr *iph,
                                      struct rte_ether_hdr *eh) {
  const uint8_t tcp_flags = tcp_h->tcp_flags;
  //  first check sequence number
  int acceptable;
  if (tcp_sk->tcb.rcv_wnd == 0) {
    acceptable = tcp_flags & (RTE_TCP_ACK_FLAG | RTE_TCP_URG_FLAG |
                              RTE_TCP_RST_FLAG);
  } else {
    acceptable =
        tcp_h->sent_seq < tcp_sk->tcb.rcv_nxt ||
        tcp_h->sent_seq > tcp_sk->tcb.rcv_nxt + tcp_sk->tcb.rcv_wnd;
  }

  if xy_unlikely (!acceptable) {
    if (tcp_flags & RTE_TCP_RST_FLAG) {
      rte_pktmbuf_free(m_buf);
      return 0;
    } else {  // send <SEQ=SND.NXT><ACK=RCV.NXT><CTL=ACK>
      return tcp_send(tcp_sk, m_buf, tcp_h, iph, eh, RTE_TCP_ACK_FLAG,
                      tcp_sk->tcb.snd_nxt, tcp_sk->tcb.rcv_nxt);
    }
  }

  // second check the RST bit
  if xy_unlikely (tcp_flags & RTE_TCP_RST_FLAG) {
    switch (tcp_sk->state) {
      case TCP_SYN_RECEIVED:
        if (tcp_sk->passive) {
          tcp_sk->state = TCP_LISTEN;
          rte_pktmbuf_free(m_buf);
          return 0;
        }
      case TCP_ESTABLISHED:
      case TCP_FIN_WAIT_1:
      case TCP_FIN_WAIT_2:
      case TCP_CLOSE_WAIT:
      case TCP_CLOSING:
      case TCP_LAST_ACK:
      case TCP_TIME_WAIT:
      default:
        tcp_sk->state = TCP_CLOSE;
        rte_pktmbuf_free(m_buf);
        return 0;
    }
  }

  // TODO third check security and precedence

  // fourth, check the SYN bit,
  if xy_unlikely (tcp_flags & RTE_TCP_SYN_FLAG) {
    tcp_sk->state = TCP_CLOSE;
    rte_pktmbuf_free(m_buf);
    return 0;
  }

  // fifth check the ACK field
  if xy_unlikely (!(tcp_flags & RTE_TCP_ACK_FLAG)) {
    rte_pktmbuf_free(m_buf);
    return 0;
  }

  switch (tcp_sk->state) {
    case TCP_SYN_RECEIVED:
      if xy_unlikely (tcp_sk->tcb.snd_una < tcp_h->recv_ack ||
                      tcp_h->recv_ack > tcp_sk->tcb.snd_nxt) {
        return tcp_send(tcp_sk, m_buf, tcp_h, iph, eh,
                        RTE_TCP_RST_FLAG, tcp_h->recv_ack, 0);
      } else {
        syn_recv_tcp_sock_dequeue(tcp_sk);
        tcp_sk->state = TCP_ESTABLISHED;
        established_tcp_sock_enqueue(tcp_sk);
      }
      tcp_sk->state = TCP_CLOSE;
      rte_pktmbuf_free(m_buf);
      return 0;
    case TCP_ESTABLISHED:
      state_tcp_handle_ack(tcp_sk, tcp_h);
      break;
    case TCP_FIN_WAIT_1:
      state_tcp_handle_ack(tcp_sk, tcp_h);
      if (tcp_h->recv_ack ==
          tcp_sk->tcb.snd_nxt) {  // TODO Way to check whether FIN is
                                  // acknowledged?
        tcp_sk->state = TCP_FIN_WAIT_2;
      }
      break;
    case TCP_FIN_WAIT_2:  // TODO??
      state_tcp_handle_ack(tcp_sk, tcp_h);
      break;
    case TCP_CLOSE_WAIT:
      state_tcp_handle_ack(tcp_sk, tcp_h);
      if (tcp_h->recv_ack ==
          tcp_sk->tcb.snd_nxt) {  // TODO Way to check whether FIN is
                                  // acknowledged?
        tcp_sk->state = TCP_TIME_WAIT;
      }
      break;
    case TCP_CLOSING:
      state_tcp_handle_ack(tcp_sk, tcp_h);
      break;
    case TCP_LAST_ACK:
      if (tcp_h->recv_ack ==
          tcp_sk->tcb.snd_nxt) {  // TODO Way to check whether FIN is
                                  // acknowledged?
        tcp_sk->state = TCP_CLOSE;  // TODO clean it
      }
      break;
    case TCP_TIME_WAIT:
      break;
  }

  // sixth, check the URG bit
  if xy_unlikely (tcp_flags & RTE_TCP_URG_FLAG) {
    switch (tcp_sk->state) {
      case TCP_ESTABLISHED:
      case TCP_FIN_WAIT_1:
      case TCP_FIN_WAIT_2:
        if (tcp_sk->tcb.rcv_up < tcp_h->tcp_urp)
          tcp_sk->tcb.rcv_up = tcp_h->tcp_urp;
        break;

      default:
        break;
    }
  }

  // seventh, process the segment text
  switch (tcp_sk->state) {
    case TCP_ESTABLISHED:
    case TCP_FIN_WAIT_1:
    case TCP_FIN_WAIT_2:
      // TODO
      break;

    default:
      break;
  }

  // eighth, check the FIN bit
  if xy_unlikely (tcp_flags & RTE_TCP_FIN_FLAG) {
    switch (tcp_sk->state) {
      case TCP_CLOSE:
      case TCP_LISTEN:
      case TCP_SYN_SENT:
        rte_pktmbuf_free(m_buf);
        return 0;
      case TCP_SYN_RECEIVED:
      case TCP_ESTABLISHED:
        tcp_sk->state = TCP_CLOSE_WAIT;
        break;
      case TCP_FIN_WAIT_1:
        if (tcp_h->recv_ack ==
            tcp_sk->tcb.snd_nxt) {  // TODO Way to check whether FIN
                                    // is acknowledged?
          tcp_sk->state = TCP_TIME_WAIT;
        } else {
          tcp_sk->state = TCP_CLOSING;
        }
      case TCP_FIN_WAIT_2:
        tcp_sk->state =
            TCP_TIME_WAIT;  // TODO Start the time-wait timer, turn
                            // off the other timers.
        break;
      case TCP_TIME_WAIT:
        // TODO Restart the time-wait timer,
        break;
      case TCP_CLOSE_WAIT:
      case TCP_CLOSING:
      case TCP_LAST_ACK:
        break;
      default:
        break;
    }
    tcp_sk->tcb.rcv_nxt = tcp_h->sent_seq + 1;
    return tcp_send(tcp_sk, m_buf, tcp_h, iph, eh, RTE_TCP_ACK_FLAG,
                    tcp_sk->tcb.snd_nxt, tcp_sk->tcb.rcv_nxt);
  }

  return 0;
}

int tcp_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh,
             struct rte_ipv4_hdr *iph, int ipv4_hdr_len, int len) {
  struct rte_tcp_hdr *tcp_h =
      (struct rte_tcp_hdr *)((unsigned char *)(iph) + ipv4_hdr_len);

  if (len < (int)(sizeof(struct rte_ether_hdr) + ipv4_hdr_len +
                  sizeof(struct rte_tcp_hdr))) {
    return 0;
  }

  if (check_tcp_hdr(tcp_h) != 0) return 0;

  xy_tcp_socket *tcp_listener =
      tcp_listener_lookup(iph->dst_addr, tcp_h->dst_port);

  if (tcp_listener == NULL) {
    return 0;
  }

  xy_tcp_socket *tcp_sk =
      tcp_sock_lookup(tcp_listener, iph->src_addr, tcp_h->src_port);

  if (tcp_sk == NULL) {
    tcp_sk = allocate_tcp_socket();
    memcpy(tcp_sk, tcp_listener, sizeof(xy_tcp_socket));
  }

  switch (tcp_sk->state) {
    case (TCP_CLOSE):
      return state_tcp_close(tcp_sk, m_buf, tcp_h, iph, eh);
    case (TCP_LISTEN):
      return state_tcp_listen(tcp_sk, m_buf, tcp_h, iph, eh);
    case TCP_SYN_SENT:
      return state_tcp_syn_sent(tcp_sk, m_buf, tcp_h, iph, eh);
    case TCP_SYN_RECEIVED:
    case TCP_ESTABLISHED:
    case TCP_FIN_WAIT_1:
    case TCP_FIN_WAIT_2:
    case TCP_CLOSE_WAIT:
    case TCP_CLOSING:
    case TCP_LAST_ACK:
    case TCP_TIME_WAIT:
      return state_tcp_otherwise(tcp_sk, m_buf, tcp_h, iph, eh);
  }

  return 0;
}
