#include <arpa/inet.h>
#include <inttypes.h>
#include <rte_arp.h>
#include <rte_byteorder.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_icmp.h>
#include <rte_ip.h>
#include <rte_ip_frag.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_tcp.h>
#include <signal.h>
#include <stdint.h>

#include "xy_api.h"
#include "xy_syn_api.h"

///  time to live
uint8_t ttl;
uint16_t xy_mtu;
uint32_t xy_this_ip;
struct rte_ether_addr xy_this_mac;

struct rte_mempool *buf_pool;

port_flag_t xy_ports_occupied[PORTS_OCCUPIED_SPACE] = {0};

static void sig_handler(int sig_num) {
  if (sig_num == SIGINT) {
    printf("\n Caught the SIGINT signal\n");
  } else {
    printf("\n Caught the signal number [%d]\n", sig_num);
  }
  exit(sig_num);
}

int xy_init_socks() {
  xy_socket_map_init();
}


int xy_dev_port_init(struct rte_mempool *buf_pool,
                     struct rte_ether_addr *eth_addr, uint16_t port,
                     uint16_t n_rx_q, uint16_t n_tx_q, uint16_t nb_rxd,
                     uint16_t nb_txd) {
  struct rte_eth_conf port_conf = {
      .rxmode = {.max_rx_pkt_len = RTE_ETHER_MAX_LEN},
      .txmode = {.mq_mode = ETH_MQ_TX_NONE},
  };

  xy_return_if(rte_eth_dev_get_mtu(port, &xy_mtu), -1);
  xy_return_if(port >= rte_eth_dev_count_avail(), -1);
  // Configure the Ethernet device.
  xy_return_if(rte_eth_dev_configure(port, n_rx_q, n_tx_q, &port_conf) != 0,
               -1);
  xy_return_if(rte_eth_dev_adjust_nb_rx_tx_desc(port, &nb_rxd, &nb_txd) != 0,
               -1);
  // Allocate and set up 1 RX queue per Ethernet port.
  for (uint16_t q = 0; q < n_rx_q; q++) {
    xy_return_if(
        rte_eth_rx_queue_setup(port, q, nb_rxd, rte_eth_dev_socket_id(port),
                               NULL, buf_pool) < 0,
        -1);
  }

  struct rte_eth_dev_info dev_info;
  rte_eth_dev_info_get(port, &dev_info);

  xy_message_if(dev_info.rx_offload_capa & DEV_RX_OFFLOAD_IPV4_CKSUM,
                "RX IPv4 checksum: support\n");
  xy_message_if(dev_info.rx_offload_capa & DEV_RX_OFFLOAD_TCP_CKSUM,
                "RX TCP  checksum: support\n");
  xy_message_if(dev_info.tx_offload_capa & DEV_TX_OFFLOAD_IPV4_CKSUM,
                "TX IPv4 checksum: support\n");
  xy_message_if(dev_info.tx_offload_capa & DEV_TX_OFFLOAD_TCP_CKSUM,
                "TX TCP  checksum: support\n");
  // Allocate and set up 1 TX queue per Ethernet port.
  for (uint16_t q = 0; q < n_tx_q; q++) {
    int ret = rte_eth_tx_queue_setup(
        port, q, nb_txd, rte_eth_dev_socket_id(port), &dev_info.default_txconf);
    xy_return_if(ret < 0, -1);
  }
  // Start the Ethernet port.
  xy_return_if(rte_eth_dev_start(port) < 0, -1);
  //  Get the port MAC address.
  rte_eth_macaddr_get(port, eth_addr);
  // Enable RX in promiscuous mode for the Ethernet device.
  // rte_eth_promiscuous_enable(port);
  rte_eth_allmulticast_enable(port);
  return 0;
}


struct rte_mempool *xy_setup(int argc, char *argv[]) {
  /* Initialize the Environment Abstraction Layer (EAL). */
  int ret = rte_eal_init(argc, argv);
  xy_rte_exit_if(ret < 0, "Error with EAL initialization\n");

  signal(SIGHUP, sig_handler);
  signal(SIGINT, sig_handler);

  /* Check that there is an even number of ports to send/receive on.
   */
  uint16_t nb_ports = rte_eth_dev_count_total();
  xy_rte_exit_if(nb_ports != 1, "Error: need 1 ports, but you have %d\n",
                 nb_ports);


  const unsigned num_bufs = 8191;
  const unsigned mbuf_cache_size = 250;

  // Creates a new mempool in memory to hold the mbufs.
  struct rte_mempool *buf_pool =
      rte_pktmbuf_pool_create("buf_pool", num_bufs * nb_ports, mbuf_cache_size,
                              0, RTE_MBUF_DEFAULT_BUF_SIZE, rte_socket_id());
  xy_rte_exit_if(buf_pool == NULL, "Cannot create mbuf pool\n");

  return buf_pool;
}


[[noreturn]] void lcore_main() {
  const uint16_t nb_ports = rte_eth_dev_count_total();
  // Check that the port is on the same NUMA node as the polling
  // thread for best performance.
  for (uint8_t port = 0; port < nb_ports; port++) {
    if xy_unlikely (rte_eth_dev_socket_id(port) > 0 &&
                    rte_eth_dev_socket_id(port) != (int)rte_socket_id())
      printf(
          "WARNING, port %u is on remote NUMA node to "
          "polling thread.\n\tPerformance will "
          "not be optimal.\n",
          port);
  }

  printf("\nCore %u forwarding packets. [Ctrl+C to quit]\n", rte_lcore_id());

  const int port = 0;
  /// Run until the application is quit or killed.
  for (;;) {
    /// Get burst of RX packets, from first port of pair.
    struct rte_mbuf *buffers[BURST_SIZE];
    xy_log("rte_eth_rx_burst\n");
    const uint16_t nb_rx = rte_eth_rx_burst(port, 0, buffers, BURST_SIZE);

    xy_log("eth_recv : %d\n", nb_rx);
    for (int i = 0; i < nb_rx; i++) {
      eth_recv(buffers[i]);
    }
    xy_log("xy_syn_event_handle\n");
    xy_syn_event_handle();
    xy_log("established_send_buffers\n");
    established_send_buffers();
  }
}
