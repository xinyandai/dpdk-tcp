#include <rte_ether.h>
#include "xy_api.h"

inline int arp_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh, int len) {
  auto *ah = (struct rte_arp_hdr *)((unsigned char *)eh + RTE_ETHER_HDR_LEN);

  if (len < (int)(sizeof(struct rte_ether_hdr) + sizeof(struct rte_arp_hdr))) {
    return 0;
  }

  if (rte_cpu_to_be_16(ah->arp_opcode) != RTE_ARP_OP_REQUEST) {  // ARP request
    return 0;
  }

  if (xy_this_ip == ah->arp_data.arp_tip) {
    rte_memcpy((unsigned char *)&eh->d_addr, (unsigned char *)&eh->s_addr, 6);
    rte_memcpy((unsigned char *)&eh->s_addr, (unsigned char *)&xy_this_mac, 6);
    ah->arp_opcode = rte_cpu_to_be_16(RTE_ARP_OP_REPLY);
    ah->arp_data.arp_tha = ah->arp_data.arp_sha;
    rte_memcpy((unsigned char *)&ah->arp_data.arp_sha,
               (unsigned char *)&xy_this_mac, 6);
    ah->arp_data.arp_tip = ah->arp_data.arp_sip;
    ah->arp_data.arp_sip = xy_this_ip;

    if (likely(1 == rte_eth_tx_burst(0, 0, &m_buf, 1))) {
      return 1;
    }
  }
  return 0;
}
