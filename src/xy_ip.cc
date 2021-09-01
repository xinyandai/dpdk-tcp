#include "xy_api.h"

inline int ip_setup(xy_ip_socket *ip_sk, struct rte_ipv4_hdr *iph,
                    uint8_t next_proto_id, rte_be16_t total_length) {
  iph->version_ihl = 0x45;   // version and header length
  iph->type_of_service = 0;  // TODO
  iph->total_length = total_length;
  iph->packet_id = rte_cpu_to_be_16(ip_sk->id++);
  iph->fragment_offset = 0x4000;       // TODO no fragmentation
  iph->time_to_live = ttl;             // time to live
  iph->next_proto_id = next_proto_id;  // tcp: iph->next_proto_id = 6
  iph->src_addr = ip_sk->ip_src;
  iph->dst_addr = ip_sk->ip_dst;

  iph->hdr_checksum = 0;
  iph->hdr_checksum = rte_ipv4_cksum(iph);  // IP header checksum TODO(cpu2be)?
  return 0;
}

int ip_send(xy_ip_socket *ip_sk, struct rte_mbuf *m_buf,
            struct rte_ipv4_hdr *iph, struct rte_ether_hdr *eh,
            uint8_t next_proto_id, uint16_t data_len) {
  const rte_be16_t total_length =
      rte_cpu_to_be_16(data_len + sizeof(struct rte_ipv4_hdr));
  ip_setup(ip_sk, iph, next_proto_id, total_length);
  return eth_send(&ip_sk->eth_socket, m_buf, eh,
                  rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4));
}

int ip_forward(xy_ip_socket *ip_sk, struct rte_mbuf *m_buf,
               struct rte_ipv4_hdr *iph, struct rte_ether_hdr *eh,
               uint8_t next_proto_id) {
  // the total_length remains since m_buf is reused
  ip_setup(ip_sk, iph, next_proto_id, iph->total_length);
  return eth_send(&ip_sk->eth_socket, m_buf, eh,
                  rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4));
}

int ip_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh, int len) {
  auto *iph =
      (struct rte_ipv4_hdr *)((unsigned char *)(eh) + RTE_ETHER_HDR_LEN);
  int ipv4_hdr_len = rte_ipv4_hdr_len(iph);

  if xy_likely (((iph->version_ihl & 0xF0) == 0x40) &&
                ((iph->fragment_offset &
                  rte_cpu_to_be_16(RTE_IPV4_HDR_OFFSET_MASK)) == 0) &&
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