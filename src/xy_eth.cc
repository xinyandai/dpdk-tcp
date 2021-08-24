#include "xy_api.h"

inline int eth_setup(xy_eth_socket *eth_sk, struct rte_ether_hdr *eh,
                     uint16_t ether_type) {
  eh->d_addr = eth_sk->mac_dst;
  eh->s_addr = eth_sk->mac_src;
  eh->ether_type = ether_type;
  return 0;
}

inline int eth_send(xy_eth_socket *eth_sk, struct rte_mbuf *m_buf,
                    struct rte_ether_hdr *eh, uint16_t ether_type) {
  eth_setup(eth_sk, eh, ether_type);

  if (likely(1 == rte_eth_tx_burst(0, 0, &m_buf, 1))) {
    return 0;
  }
  return -1;
}

inline int eth_recv(struct rte_mbuf *buf) {
  int len = rte_pktmbuf_data_len(buf);
  struct rte_ether_hdr *eh = rte_pktmbuf_mtod(buf, struct rte_ether_hdr *);

  if (eh->ether_type ==
      rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4)) {  // IPv4 protocol
    return ip_recv(buf, eh, len);
  } else if (eh->ether_type ==
             rte_cpu_to_be_16(RTE_ETHER_TYPE_ARP)) {  // ARP protocol
    return arp_recv(buf, eh, len);
  }

  rte_pktmbuf_free(buf);
  return 0;
}