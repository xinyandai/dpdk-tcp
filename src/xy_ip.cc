#include "xy_api.h"

int ip_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh, int len) {
  struct rte_ipv4_hdr *iph =
      (struct rte_ipv4_hdr *)((unsigned char *)(eh) + RTE_ETHER_HDR_LEN);
  int ipv4_hdr_len = (iph->version_ihl & RTE_IPV4_HDR_IHL_MASK) << 2;

  if (((iph->version_ihl & 0xF0) == 0x40) &&
      ((iph->fragment_offset & rte_cpu_to_be_16(RTE_IPV4_HDR_OFFSET_MASK)) ==
       0) &&
      (iph->dst_addr == xy_this_ip)) {
    // ipv4
    if (iph->next_proto_id == IPPROTO_TCP) {  // TCP
      return tcp_recv(m_buf, eh, iph, ipv4_hdr_len, len);
    } else if (iph->next_proto_id == 1) {  // ICMP
      return icmp_recv(m_buf, eh, iph, ipv4_hdr_len, len);
    }
  }
  return 0;
}

int ip_send(xy_ip_socket *ip_sk, struct rte_mbuf *m_buf,
            struct rte_ipv4_hdr *iph, struct rte_ether_hdr *eh,
            uint8_t next_proto_id) {
  iph->version_ihl = 0x45;             // version and header length
  iph->type_of_service = 0;            // TODO
  iph->total_length;                   // TODO /**< length of packet */
  iph->packet_id;                      // TODO /**< packet ID */
  iph->fragment_offset;                // TODO /**< fragmentation offset */
  iph->time_to_live = ttl;             // time to live
  iph->next_proto_id = next_proto_id;  // tcp: iph->next_proto_id = 6

  iph->src_addr = ip_sk->ip_src;
  iph->dst_addr = ip_sk->ip_dst;

  iph->hdr_checksum = rte_ipv4_cksum(iph);  // IP header checksum

  return eth_send(&ip_sk->eth_socket, m_buf, eh,
                  rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4));
}