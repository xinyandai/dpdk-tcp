#include "xy_api.h"

static inline int tcp_send_one_buf(xy_tcp_socket *tcp_sk,
                                   struct rte_mbuf *m_buf) {
  struct rte_ether_hdr *eh = rte_pktmbuf_mtod(m_buf, struct rte_ether_hdr *);
  auto iph = (struct rte_ipv4_hdr *)((unsigned char *)(eh) + RTE_ETHER_HDR_LEN);
  auto tcp_h = (struct rte_tcp_hdr *)((unsigned char *)(iph) + XY_IP_HDR_LEN);

  tcp_ready(tcp_h, iph, tcp_sk->tcb->snd_nxt++, 0);
  return ip_forward(&tcp_sk->ip_socket, m_buf, iph, eh, IPPROTO_TCP);
}

int tcp_send_buf(xy_tcp_socket *tcp_sk) {
  /**
   * TODO
   * 1. timeout
   * 2. batch send
   * 3. batch add to tcb->snd_wnd_buffer
   */
  struct tcb *tcb = tcp_sk->tcb;
  xy_tcp_snd_wnd *window = &tcb->snd_wnd_buffer;
  uint16_t size = xy_tcp_snd_wnd_size(window);

  /// timeout
  if (size > 0) {
    tcp_send_one_buf(tcp_sk, window->buffer[window->head_]);
  }

  for (; size < tcb->snd_wnd; ++size) {
    struct rte_mbuf *mbuf = tcp_send_dequeue(tcp_sk);
    xy_tcp_snd_wnd_add(&tcb->snd_wnd_buffer, mbuf);
    tcp_send_one_buf(tcp_sk, mbuf);
  }
  return 0;
}