#include "xy_api.h"

inline int icmp_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh,
                     struct rte_ipv4_hdr *iph, int ipv4_hdr_len, int len) {
  struct rte_icmp_hdr *icmp_hdr =
      (struct rte_icmp_hdr *)((unsigned char *)(iph) + ipv4_hdr_len);

  if (len < (int)(sizeof(struct rte_ether_hdr) + sizeof(struct rte_icmp_hdr))) {
    return 0;
  }

  if ((icmp_hdr->icmp_type == RTE_IP_ICMP_ECHO_REQUEST) &&
      (icmp_hdr->icmp_code == 0)) {  // ICMP echo req
    rte_memcpy((unsigned char *)&eh->d_addr, (unsigned char *)&eh->s_addr, 6);
    rte_memcpy((unsigned char *)&eh->s_addr, (unsigned char *)&xy_this_mac, 6);
    iph->dst_addr = iph->src_addr;
    iph->src_addr = xy_this_ip;
    iph->time_to_live = ttl;
    iph->hdr_checksum = rte_ipv4_cksum(iph);
    icmp_hdr->icmp_type = RTE_IP_ICMP_ECHO_REPLY;
    icmp_hdr->icmp_cksum = 0;
    icmp_hdr->icmp_cksum =
        ~rte_raw_cksum(icmp_hdr, len - RTE_ETHER_HDR_LEN - ipv4_hdr_len);
    if(0 == rte_eth_tx_burst(0, 0, &m_buf, 1)) {
      return 1;
    }
  }
  return 0;
}