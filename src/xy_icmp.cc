#include "xy_api.h"

inline int 
process_icmp(struct rte_mbuf *mbuf, 
             struct ether_hdr *eh, 
             struct ipv4_hdr *iph,
			 int ipv4_hdrlen, int len) {
	struct icmp_hdr *icmph = (struct icmp_hdr *)((unsigned char *)(iph) + ipv4_hdrlen);

	if (len < (int)(sizeof(struct ether_hdr) + sizeof(struct icmp_hdr))) {
		return 0;
	}

	if ((icmph->icmp_type == IP_ICMP_ECHO_REQUEST) && (icmph->icmp_code == 0)) {	// ICMP echo req
		rte_memcpy((unsigned char *)&eh->d_addr, (unsigned char *)&eh->s_addr, 6);
		rte_memcpy((unsigned char *)&eh->s_addr, (unsigned char *)&my_eth_addr, 6);
		iph->dst_addr = iph->src_addr;
		iph->src_addr = __xy_this_ip;
		iph->time_to_live = TTL;
		iph->hdr_checksum = 0;
		iph->hdr_checksum = rte_ipv4_cksum(iph);
		icmph->icmp_type = IP_ICMP_ECHO_REPLY;
		icmph->icmp_cksum = 0;
		icmph->icmp_cksum = ~rte_raw_cksum(icmph, len - ETHER_HDR_LEN - ipv4_hdrlen);
		return rte_eth_tx_burst(0, 0, &mbuf, 1);
	}
	return 0;
}