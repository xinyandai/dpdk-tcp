#pragma once
#ifndef __DPDK_TCP_INCLUDE_XY_API_H__
#define __DPDK_TCP_INCLUDE_XY_API_H__

#include <sys/socket.h>
#include <sys/types.h>

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

#include "xy_ip.h"
#include "xy_arp.h"
#include "xy_eth.h"
#include "xy_tcp.h"
#include "xy_icmp.h"

#include "xy_list.h"

#include "xy_socks.h"
#include "xy_macros.h"
#include "xy_struct.h"
#include "xy_tcp_port.h"

#include "xy_tcp_wnd_hdl.h"

extern uint8_t ttl;
extern uint32_t xy_this_ip;
extern struct rte_ether_addr xy_this_mac;

extern struct rte_mempool *buf_pool;

#define BURST_SIZE 1024
#define tcp_sock_t xy_tcp_socket *

[[noreturn]] void lcore_main();

struct rte_mempool *xy_setup(int argc, char *argv[]);

int xy_dev_port_init(struct rte_mempool *buf_pool,
                     struct rte_ether_addr *eth_addr, uint16_t port,
                     uint16_t n_rx_q, uint16_t n_tx_q, uint16_t nb_rxd,
                     uint16_t nb_txd);

tcp_sock_t xy_socket(int domain, int type, int protocol);
int xy_bind(tcp_sock_t tcp_sk, uint32_t ip, uint16_t port);
int xy_listen(tcp_sock_t tcp_sk, int backlog);
tcp_sock_t xy_accept(tcp_sock_t tcp_sk, uint32_t *ip, uint16_t *port);
tcp_sock_t connect(tcp_sock_t tcp_sk, uint32_t *ip, uint16_t *port);
ssize_t xy_recv(tcp_sock_t tcp_sk, char *buf, size_t len, int flags);
ssize_t xy_send(tcp_sock_t tcp_sk, const char *buf, size_t len);
int xy_close(tcp_sock_t tcp_sk);

#endif