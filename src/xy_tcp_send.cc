#include "xy_api.h"

static inline int tcp_send_one_buf(xy_tcp_socket *tcp_sk,
                                   struct rte_mbuf *buf) {
  struct rte_mbuf *m_buf = rte_pktmbuf_alloc(buf_pool);
  struct rte_ether_hdr *eh = rte_pktmbuf_mtod(m_buf, struct rte_ether_hdr *);
  auto iph = (struct rte_ipv4_hdr *)((unsigned char *)(eh) + RTE_ETHER_HDR_LEN);
  auto tcp_h = (struct rte_tcp_hdr *)((unsigned char *)(iph) + XY_IP_HDR_LEN);

  tcp_ready(tcp_h, iph, tcp_sk->tcb.snd_nxt++, 0);
  return ip_forward(&tcp_sk->ip_socket, m_buf, iph, eh, IPPROTO_TCP);
}

int tcp_send_buf(xy_tcp_socket *tcp_sk) {
  struct rte_mbuf *m_buf = NULL;

  while (NULL != (m_buf = send_dequeue(tcp_sk))) {
    tcp_send_one_buf(tcp_sk, m_buf);
  }

  return 0;
}