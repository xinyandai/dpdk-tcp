#include "xy_api.h"

inline int tcp_setup(xy_tcp_socket *tcp_sk, struct rte_tcp_hdr *tcp_h,
                     uint8_t tcp_flags) {
  tcp_h->dst_port = tcp_sk->port_dst;
  tcp_h->src_port = tcp_sk->port_src;

  tcp_h->data_off = sizeof(struct rte_tcp_hdr) / 4;
  tcp_h->tcp_flags = tcp_flags;
  tcp_h->rx_win = tcp_sk->tcb->rcv_wnd;
  tcp_h->tcp_urp = 0;  // TODO
  return 0;
}

inline int tcp_ready(struct rte_tcp_hdr *tcp_h, struct rte_ipv4_hdr *iph,
                     uint32_t sent_seq, uint32_t recv_ack) {
  tcp_h->sent_seq = rte_cpu_to_be_32(sent_seq);
  tcp_h->recv_ack = rte_cpu_to_be_32(recv_ack);

  /// Byte Order Independence as in rfc1071
  tcp_h->cksum = 0;
  tcp_h->cksum = rte_ipv4_udptcp_cksum(iph, tcp_h);
  return 0;
}

int tcp_forward(xy_tcp_socket *tcp_sk, struct rte_mbuf *m_buf,
                struct rte_tcp_hdr *tcp_h, struct rte_ipv4_hdr *iph,
                struct rte_ether_hdr *eh, uint8_t tcp_flags,
                rte_be32_t sent_seq, rte_be32_t recv_ack) {
  tcp_setup(tcp_sk, tcp_h, tcp_flags);
  tcp_ready(tcp_h, iph, sent_seq, recv_ack);
  return ip_forward(&tcp_sk->ip_socket, m_buf, iph, eh, IPPROTO_TCP);
}

int tcp_send(xy_tcp_socket *tcp_sk, struct rte_mbuf *m_buf, uint8_t tcp_flags,
             rte_be32_t sent_seq, rte_be32_t recv_ack, uint16_t data_len) {
  struct rte_ether_hdr *eh = rte_pktmbuf_mtod(m_buf, struct rte_ether_hdr *);
  struct rte_ipv4_hdr *iph =
      (struct rte_ipv4_hdr *)((unsigned char *)(eh) + RTE_ETHER_HDR_LEN);
  struct rte_tcp_hdr *tcp_h =
      (struct rte_tcp_hdr *)((unsigned char *)(iph) +
                             sizeof(struct rte_ipv4_hdr));

  tcp_setup(tcp_sk, tcp_h, tcp_flags);
  tcp_ready(tcp_h, iph, sent_seq, recv_ack);
  return ip_send(&tcp_sk->ip_socket, m_buf, iph, eh, IPPROTO_TCP,
                 data_len + sizeof(struct rte_tcp_hdr));
}

int tcp_send(xy_tcp_socket *tcp_sk, struct rte_mbuf *m_buf,
             struct rte_tcp_hdr *tcp_h, struct rte_ipv4_hdr *iph,
             struct rte_ether_hdr *eh, uint8_t tcp_flags, rte_be32_t sent_seq,
             rte_be32_t recv_ack, uint16_t data_len) {
  tcp_setup(tcp_sk, tcp_h, tcp_flags);
  tcp_ready(tcp_h, iph, sent_seq, recv_ack);
  return ip_send(&tcp_sk->ip_socket, m_buf, iph, eh, IPPROTO_TCP,
                 data_len + sizeof(struct rte_tcp_hdr));
}