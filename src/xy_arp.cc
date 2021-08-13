#include "xy_api.h"

inline int process_arp(struct rte_mbuf *mbuf, struct ether_hdr *eh, int len) {
	struct arp_hdr *ah = (struct arp_hdr *)((unsigned char *)eh + ETHER_HDR_LEN);

	if (len < (int)(sizeof(struct ether_hdr) + sizeof(struct arp_hdr))) {
		return 0;
	}
	if (rte_cpu_to_be_16(ah->arp_op) != ARP_OP_REQUEST) {	// ARP request
		return 0;
	}
	if (__xy_this_ip == ah->arp_data.arp_tip) {
		rte_memcpy((unsigned char *)&eh->d_addr, (unsigned char *)&eh->s_addr, 6);
		rte_memcpy((unsigned char *)&eh->s_addr, (unsigned char *)&my_eth_addr, 6);
		ah->arp_op = rte_cpu_to_be_16(ARP_OP_REPLY);
		ah->arp_data.arp_tha = ah->arp_data.arp_sha;
		rte_memcpy((unsigned char *)&ah->arp_data.arp_sha, (unsigned char *)&my_eth_addr, 6);
		ah->arp_data.arp_tip = ah->arp_data.arp_sip;
		ah->arp_data.arp_sip = __xy_this_ip;

		if (likely(1 == rte_eth_tx_burst(0, 0, &mbuf, 1))) {
			send_arp_pkts++;
			return 1;
		}
	}
	return 0;
}
