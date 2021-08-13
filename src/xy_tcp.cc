#include "xy_api.h"

static int _check_tcp_hdr(struct tcp_hdr *tcph) {
    // port
    // ip
    return 0;
}

inline int 
process_tcp(struct rte_mbuf *mbuf, 
            struct ether_hdr *eh, 
            struct ipv4_hdr *iph,
			int ipv4_hdrlen, int len) {
    
    
	struct tcp_hdr *tcph = (struct tcp_hdr *)((unsigned char *)(iph) + ipv4_hdrlen);
    int pkt_len = ipv4_hdrlen + sizeof(struct tcp_hdr);
	
    if (len < (int)(sizeof(struct ether_hdr) + ipv4_hdrlen + sizeof(struct tcp_hdr))) {
		return 0;
	}

	if (_check_tcp_hdr(tcph) != 0)
		return 0;

	if ((tcph->tcp_flags & (TCP_SYN | TCP_ACK)) == TCP_SYN) {	// SYN packet, send SYN+ACK


		swap_6bytes((unsigned char *)&eh->s_addr, (unsigned char *)&eh->d_addr);
		swap_4bytes((unsigned char *)&iph->src_addr, (unsigned char *)&iph->dst_addr);
		swap_2bytes((unsigned char *)&tcph->src_port, (unsigned char *)&tcph->dst_port);
		tcph->tcp_flags = TCP_ACK | TCP_SYN;
		tcph->recv_ack = rte_cpu_to_be_32(rte_be_to_cpu_32(tcph->sent_seq) + 1);
		tcph->sent_seq =
		    rte_cpu_to_be_32(*(uint32_t *) & iph->src_addr +
				     *(uint32_t *) & iph->dst_addr +
				     *(uint16_t *) & tcph->src_port +
				     *(uint16_t *) & tcph->dst_port + tcp_syn_random);
		tcph->data_off = (sizeof(struct tcp_hdr) / 4) << 4;
		tcph->cksum = 0;
		
		iph->total_length = rte_cpu_to_be_16(pkt_len);
		iph->hdr_checksum = 0;
		iph->time_to_live = TTL;
		rte_pktmbuf_data_len(mbuf) = rte_pktmbuf_pkt_len(mbuf) = pkt_len + ETHER_HDR_LEN;
		if (hardware_cksum) {
			mbuf->ol_flags = PKT_TX_IPV4 | PKT_TX_IP_CKSUM | PKT_TX_TCP_CKSUM;
			mbuf->l2_len = ETHER_HDR_LEN;
			mbuf->l3_len = ipv4_hdrlen;
			mbuf->l4_len = 0;
			tcph->cksum = rte_ipv4_phdr_cksum((const struct ipv4_hdr *)iph, 0);
		} else {
			tcph->cksum = rte_ipv4_udptcp_cksum(iph, tcph);
			iph->hdr_checksum = rte_ipv4_cksum(iph);
		}
		int ret = rte_eth_tx_burst(0, 0, &mbuf, 1);
		if (ret == 1)
			return 1;
		return 0;
	} 
    else if (tcph->tcp_flags & TCP_FIN) {	// FIN packet, send ACK

		recv_tcp_fin_pkts++;
		swap_6bytes((unsigned char *)&eh->s_addr, (unsigned char *)&eh->d_addr);
		swap_4bytes((unsigned char *)&iph->src_addr, (unsigned char *)&iph->dst_addr);
		swap_2bytes((unsigned char *)&tcph->src_port, (unsigned char *)&tcph->dst_port);
		swap_4bytes((unsigned char *)&tcph->sent_seq, (unsigned char *)&tcph->recv_ack);
		tcph->tcp_flags = TCP_ACK;
		tcph->recv_ack = rte_cpu_to_be_32(rte_be_to_cpu_32(tcph->recv_ack) + 1);
		tcph->data_off = (sizeof(struct tcp_hdr) / 4) << 4;
		tcph->cksum = 0;
		
		iph->total_length = rte_cpu_to_be_16(pkt_len);
		iph->hdr_checksum = 0;
		iph->time_to_live = TTL;
		rte_pktmbuf_data_len(mbuf) = rte_pktmbuf_pkt_len(mbuf) = pkt_len + ETHER_HDR_LEN;
		if (hardware_cksum) {
			// printf("ol_flags=%ld\n",mbuf->ol_flags);
			mbuf->ol_flags = PKT_TX_IPV4 | PKT_TX_IP_CKSUM | PKT_TX_TCP_CKSUM;
			mbuf->l2_len = ETHER_HDR_LEN;
			mbuf->l3_len = ipv4_hdrlen;
			mbuf->l4_len = 0;
			tcph->cksum = rte_ipv4_phdr_cksum((const struct ipv4_hdr *)iph, 0);
		} else {
			tcph->cksum = rte_ipv4_udptcp_cksum(iph, tcph);
			iph->hdr_checksum = rte_ipv4_cksum(iph);
		}

		int ret = rte_eth_tx_burst(0, 0, &mbuf, 1);
		if (ret == 1)
			return 1;
		return 0;
	} else if ((tcph->tcp_flags & (TCP_SYN | TCP_ACK)) == TCP_ACK) {	// ACK packet, send DATA
		pkt_len = rte_be_to_cpu_16(iph->total_length);
		int tcp_payload_len = pkt_len - ipv4_hdrlen - (tcph->data_off >> 4) * 4;
		int ntcp_payload_len = MAXIPLEN;
		unsigned char *tcp_payload;
		unsigned char buf[MAXIPLEN + sizeof(struct tcp_hdr)];	// http_response
		int resp_in_req = 0;
		recv_tcp_data_pkts++;

		if (tcp_payload_len <= 5) {
			return 0;
		}
		if (tcph->recv_ack !=
		    rte_cpu_to_be_32(*(uint32_t *) & iph->src_addr +
				     *(uint32_t *) & iph->dst_addr +
				     *(uint16_t *) & tcph->src_port +
				     *(uint16_t *) & tcph->dst_port + tcp_syn_random + 1)) {
			return 0;
		}
		tcp_payload = (unsigned char *)iph + ipv4_hdrlen + (tcph->data_off >> 4) * 4;
		if (process_http
		    (4, iph, tcph, tcp_payload, tcp_payload_len, buf + sizeof(struct tcp_hdr),
		     &ntcp_payload_len, &resp_in_req) == 0)
			return 0;
		uint32_t ack_seq =
		    rte_cpu_to_be_32(rte_be_to_cpu_32(tcph->sent_seq) + tcp_payload_len);
		swap_6bytes((unsigned char *)&eh->s_addr, (unsigned char *)&eh->d_addr);
		swap_4bytes((unsigned char *)&iph->src_addr, (unsigned char *)&iph->dst_addr);
		swap_2bytes((unsigned char *)&tcph->src_port, (unsigned char *)&tcph->dst_port);
		tcph->tcp_flags = TCP_ACK | TCP_PSH | TCP_FIN;
		tcph->sent_seq = tcph->recv_ack;
		tcph->recv_ack = ack_seq;
		tcph->cksum = 0;
		iph->hdr_checksum = 0;
		iph->time_to_live = TTL;

		if (ntcp_payload_len <= TCPMSS) {	// tcp packet fit in one IP packet
			if (!resp_in_req)
				rte_memcpy(tcp_payload, buf + sizeof(struct tcp_hdr),
					   ntcp_payload_len);
			int pkt_len = ntcp_payload_len + ipv4_hdrlen + (tcph->data_off >> 4) * 4;
			iph->total_length = rte_cpu_to_be_16(pkt_len);
			iph->fragment_offset = 0;

			rte_pktmbuf_data_len(mbuf) = rte_pktmbuf_pkt_len(mbuf) =
			    pkt_len + ETHER_HDR_LEN;
			if (hardware_cksum) {
				// printf("ol_flags=%ld\n",mbuf->ol_flags);
				mbuf->ol_flags = PKT_TX_IPV4 | PKT_TX_IP_CKSUM | PKT_TX_TCP_CKSUM;
				mbuf->l2_len = ETHER_HDR_LEN;
				mbuf->l3_len = ipv4_hdrlen;
				mbuf->l4_len = ntcp_payload_len;
				tcph->cksum = rte_ipv4_phdr_cksum((const struct ipv4_hdr *)iph, 0);
			} else {
				tcph->cksum = rte_ipv4_udptcp_cksum(iph, tcph);
				iph->hdr_checksum = rte_ipv4_cksum(iph);
			}

			int ret = rte_eth_tx_burst(0, 0, &mbuf, 1);
			if (ret == 1) {
				send_tcp_data_pkts++;
				return 1;
			}

			return 0;
		} 
        else {	// tcp packet could not fit in one IP packet, I will send one by one
			struct rte_mbuf *frag;
			struct ether_hdr *neh;
			struct ipv4_hdr *niph;
			struct tcp_hdr *ntcph;
			int left = ntcp_payload_len + sizeof(struct tcp_hdr);
			uint32_t offset = 0;
			if (resp_in_req) {
				printf("BIG TCP packet, must returned in my buf\n");
				return 0;
			}
			iph->total_length = rte_cpu_to_be_16(left + sizeof(struct ipv4_hdr));
			iph->fragment_offset = 0;
			iph->packet_id = tcph->dst_port;
			tcph->data_off = (sizeof(struct tcp_hdr) / 4) << 4;
			ntcph = (struct tcp_hdr *)buf;
			rte_memcpy(ntcph, tcph, sizeof(struct tcp_hdr));	// copy tcp header to begin of buf
			ntcph->cksum = rte_ipv4_udptcp_cksum(iph, ntcph);	// trick but works, now eth/ip header in mbuf, tcp packet in buf
			while (left > 0) {
				len = left < TCPMSS ? left : (TCPMSS & 0xfff0);
				left -= len;

				frag = rte_pktmbuf_alloc(mbuf_pool);
				if (!frag) {
					printf("mutli packet alloc error\n");
					return 0;
				}
				neh = rte_pktmbuf_mtod(frag, struct ether_hdr *);
				rte_memcpy(neh, eh, ETHER_HDR_LEN + sizeof(struct ipv4_hdr));	// copy eth/ip header
				niph = (struct ipv4_hdr *)((unsigned char *)(neh) + ETHER_HDR_LEN);
				ntcph =
				    (struct tcp_hdr *)((unsigned char *)(niph) +
						       sizeof(struct ipv4_hdr));
				rte_memcpy(ntcph, buf + offset, len);

				pkt_len = len + sizeof(struct ipv4_hdr);
				niph->total_length = rte_cpu_to_be_16(pkt_len);
				niph->fragment_offset = rte_cpu_to_be_16(offset >> 3);
				if (left > 0)
					niph->fragment_offset |= rte_cpu_to_be_16(IPV4_HDR_MF_FLAG);
#ifdef DEBUGTCP
				fprintf(stderr, "frag offset %d, pkt len=%d\n", offset, pkt_len);
#endif
				rte_pktmbuf_data_len(frag) = rte_pktmbuf_pkt_len(frag) =
				    pkt_len + ETHER_HDR_LEN;
				if (hardware_cksum) {
					// printf("ol_flags=%ld\n", frag->ol_flags);
					frag->ol_flags = PKT_TX_IPV4 | PKT_TX_IP_CKSUM;
					frag->l2_len = ETHER_HDR_LEN;
					frag->l3_len = sizeof(struct ipv4_hdr);
					frag->l4_len = len;
				} else
					niph->hdr_checksum = rte_ipv4_cksum(niph);

#ifdef DEBUGTCP
//                              printf("I will reply following:\n");
//                              dump_packet((unsigned char *)neh, rte_pktmbuf_data_len(frag));
#endif
				int ret = rte_eth_tx_burst(0, 0, &frag, 1);
				if (ret != 1) {
#ifdef DEBUGTCP
					fprintf(stderr, "send tcp packet return %d\n", ret);
#endif
					rte_pktmbuf_free(frag);
					return 0;
				}
				send_tcp_data_multi_pkts++;
				offset += len;
			}
			rte_pktmbuf_free(mbuf);
			return 1;
		}
	}
	return 0;
}