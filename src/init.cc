#include "xy_api.h"



uint32_t __xy_this_ip;

/*
 * Initializes a given port using global settings and with the RX buffers
 * coming from the mbuf_pool passed as a parameter.
 */
static inline int 
xy_dev_port_init(struct rte_mempool *mbuf_pool, 
				 struct ether_addr *eth_addr, uint16_t port,
				 uint16_t rx_rings, uint16_t tx_rings, 
				 uint16_t nb_rxd, uint16_t nb_txd, 
				 unsigned num_bufs, unsigned mbuf_cache_size) {
	struct rte_eth_conf port_conf = = {
		.rxmode = {.max_rx_pkt_len = ETHER_MAX_LEN},
		.txmode = {.mq_mode = ETH_MQ_TX_NONE},
	};
	
	xy_return_if (port >= rte_eth_dev_count(), -1);
	
	// Configure the Ethernet device.
	xy_return_if (rte_eth_dev_configure(port, rx_rings, tx_rings, &port_conf) != 0, -1);
	xy_return_if (rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd) != 0, -1);

	/* Allocate and set up 1 RX queue per Ethernet port. */
	for (uint16_t q = 0; q < rx_rings; q++) {
		xy_return_if (rte_eth_rx_queue_setup(port, q, nb_rxd, 
		                rte_eth_dev_socket_id(port), NULL, mbuf_pool) < 0, -1);
	}

	struct rte_eth_dev_info dev_info;
	rte_eth_dev_info_get(port, &dev_info);
	xy_message_if (dev_info.rx_offload_capa & DEV_RX_OFFLOAD_IPV4_CKSUM, "RX IPv4 checksum: support\n");
	xy_message_if (dev_info.rx_offload_capa & DEV_RX_OFFLOAD_TCP_CKSUM,  "RX TCP  checksum: support\n");
	xy_message_if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_IPV4_CKSUM, "TX IPv4 checksum: support\n");
	xy_message_if (dev_info.tx_offload_capa & DEV_TX_OFFLOAD_TCP_CKSUM,  "TX TCP  checksum: support\n");

	// hardware checksum is not used

	/* Dsiable features that are not supported by port's HW */
	if (!(dev_info.tx_offload_capa & DEV_TX_OFFLOAD_TCP_CKSUM))
		dev_info.default_txconf.txq_flags |= ETH_TXQ_FLAGS_NOXSUMTCP;

	/* Allocate and set up 1 TX queue per Ethernet port. */
	for (uint16_t q = 0; q < tx_rings; q++) {
		xy_return_if (rte_eth_tx_queue_setup(port, q, nb_txd,
		                rte_eth_dev_socket_id(port), &dev_info.default_txconf) < 0, -1);
		
	}

	// Start the Ethernet port.
	xy_return_if (rte_eth_dev_start(port) < 0, -1);

	//  Get the port MAC address.
	rte_eth_macaddr_get(port, eth_addr);

	// Enable RX in promiscuous mode for the Ethernet device.
	// rte_eth_promiscuous_enable(port);
	rte_eth_allmulticast_enable(port);
	return 0;
}

/*
 * The main function, which does initialization and calls the per-lcore
 * functions.
 */
int xy_tcp_setup(int argc, char *argv[]) {
	/* Initialize the Environment Abstraction Layer (EAL). */
	int ret = rte_eal_init(argc, argv);
	xy_rte_exit_if (ret < 0, "Error with EAL initialization\n");

	signal(SIGHUP, sig_handler_hup);

	/* Check that there is an even number of ports to send/receive on. */
	xy_rte_exit_if (rte_eth_dev_count() != 1, "Error: need 1 ports, but you have %d\n", nb_ports);

	xy_warn_if (rte_lcore_count() > 1, "\nWARNING: Too many lcores enabled. Only 1 used.\n");
	
	// Creates a new mempool in memory to hold the mbufs.
	struct rte_mempool *mbuf_pool = rte_pktmbuf_pool_create(
		"MBUF_POOL", num_bufs * nb_ports, mbuf_cache_size, 0,
		RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
	xy_rte_exit_if(mbuf_pool == NULL, "Cannot create mbuf pool\n");

	return ret;
}

void process_eth_package(struct rte_mbuf *buf) {
	int len = rte_pktmbuf_data_len(buf);
	struct ether_hdr *eh = rte_pktmbuf_mtod(buf, struct ether_hdr *);

	if (eh->ether_type == rte_cpu_to_be_16(ETHER_TYPE_IPv4)) {	// IPv4 protocol
		struct ipv4_hdr *iph = (struct ipv4_hdr *)((unsigned char *)(eh) + ETHER_HDR_LEN);
		int ipv4_hdrlen = (iph->version_ihl & IPV4_HDR_IHL_MASK) << 2;

		if (((iph->version_ihl & 0xF0) == 0x40) && ((iph->fragment_offset & rte_cpu_to_be_16(IPV4_HDR_OFFSET_MASK)) == 0) && (iph->dst_addr == my_ip)) {	// ipv4
			if (iph->next_proto_id == 6) {	// TCP
				process_tcp(bufs[i], eh, iph, ipv4_hdrlen, len);
			} else if (iph->next_proto_id == 1) {	// ICMP
				process_icmp(bufs[i], eh, iph, ipv4_hdrlen, len));
			}
		}
	} else if (eh->ether_type == rte_cpu_to_be_16(ETHER_TYPE_ARP)) {	// ARP protocol
		process_arp(bufs[i], eh, len);
	} 
	rte_pktmbuf_free(buf);
}

/*
 * The lcore main. This is the main thread that does the work, reading from
 * an input port and writing to an output port.
 */
static __attribute__ ((noreturn))
void lcore_main(void) {
	const uint8_t nb_ports = rte_eth_dev_count();
	// Check that the port is on the same NUMA node as the polling thread
	// for best performance.
	for (uint8_t port = 0; port < nb_ports; port++)
		if (rte_eth_dev_socket_id(port) > 0
		    && rte_eth_dev_socket_id(port) != (int)rte_socket_id())
			printf("WARNING, port %u is on remote NUMA node to "
			       "polling thread.\n\tPerformance will " "not be optimal.\n", port);

	printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n", rte_lcore_id());

	/* Run until the application is quit or killed. */
	for (;;) {
		/* Get burst of RX packets, from first port of pair. */
		const int port = 0;
		struct rte_mbuf *bufs[BURST_SIZE];
		const uint16_t nb_rx = rte_eth_rx_burst(port, 0, bufs, BURST_SIZE);
		if (unlikely(nb_rx == 0))
			continue;
			
		for (int i = 0; i < nb_rx; i++) {

		}
	}
}

