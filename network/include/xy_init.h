//
// Created by xydai on 2021/9/10.
//
#pragma once
#ifndef DPDK_TCP_NETWORK_INCLUDE_XY_INIT_H_
#define DPDK_TCP_NETWORK_INCLUDE_XY_INIT_H_
#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
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

extern uint8_t ttl;
extern uint32_t xy_this_ip;
extern struct rte_ether_addr xy_this_mac;

extern struct rte_mempool *buf_pool;

#define BURST_SIZE 1024

/**
 * The lcore main. This is the main thread that does the work, reading
 * from an input port and writing to an output port.
 */
[[noreturn]] void lcore_main();

int xy_init_socks();

struct rte_mempool *xy_setup(int argc, char *argv[]);
/**
 * Initializes a given port using global settings and with the RX
 * buffers coming from the buf_pool passed as a parameter.
 * \param buf_pool
 * \param eth_addr
 * \param port
 * \param n_rx_q
 * \param n_tx_q
 * \param nb_rxd
 * \param nb_txd
 * \return
 */
int xy_dev_port_init(struct rte_mempool *buf_pool,
                     struct rte_ether_addr *eth_addr, uint16_t port,
                     uint16_t n_rx_q, uint16_t n_tx_q, uint16_t nb_rxd,
                     uint16_t nb_txd);


#endif  // DPDK_TCP_NETWORK_INCLUDE_XY_INIT_H_
