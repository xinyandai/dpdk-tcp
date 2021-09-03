#include "xy_api.h"
#include "xy_syn_api.h"
#include "xy_tcp_state_close.h"
#include "xy_tcp_state_listen.h"
#include "xy_tcp_state_others.h"
#include "xy_tcp_state_syn_sent.h"

/** check ip address and port
 * \param tcp_h
 * \return 0 if valid
 */
static int check_tcp_hdr(struct rte_tcp_hdr *tcp_h, struct rte_ipv4_hdr *iph) {
  return tcp_h->cksum == rte_ipv4_udptcp_cksum(iph, tcp_h) ? 0 : -1;
}

int tcp_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh,
             struct rte_ipv4_hdr *iph, int ipv4_hdr_len, int len) {
  struct rte_tcp_hdr *tcp_h =
      (struct rte_tcp_hdr *)((unsigned char *)(iph) + ipv4_hdr_len);

  if (len < (int)(sizeof(struct rte_ether_hdr) + ipv4_hdr_len +
                  sizeof(struct rte_tcp_hdr))) {
    return 0;
  }

  if (check_tcp_hdr(tcp_h, iph) != 0) return 0;

  xy_tcp_socket *tcp_sk = NULL;
  {  /// socket lookup
    tcp_sk = syn_rcvd_tcp_sock_lookup(iph->dst_addr, tcp_h->dst_port,
                                      iph->src_addr, tcp_h->src_port);

    if (tcp_sk == NULL) {
      tcp_sk = established_tcp_sock_look_up(iph->dst_addr, tcp_h->dst_port,
                                            iph->src_addr, tcp_h->src_port);
    }

    if (tcp_sk == NULL) {
      xy_tcp_socket *tcp_listener =
          listener_tcp_sock_lookup(iph->dst_addr, tcp_h->dst_port);

      if (tcp_listener == NULL) {
        return 0;
      }

      tcp_sk = tcp_listener;
    }
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
