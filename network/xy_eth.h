//
// Created by xydai on 2021/8/20.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_ETH_H_
#define DPDK_TCP_INCLUDE_XY_ETH_H_
#include <rte_mbuf.h>

#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>

#include "xy_struct.h"

/**
 * \brief process the received ether package. May call the IP, ARP subroutine to
 * process the package
 * \param buf the received rte_mbuf
 * \return 0 if handled successfully, 1 on error
 */
int eth_recv(struct rte_mbuf *buf);

/**
 * Fill up the ether header with available information in eth_sk.
 * \param eth_sk socket including source and destination mac addresses.
 * \param eh ether header
 * \param ether_type encapsulated protocol, in Big Endian
 * \return
 */
int eth_setup(xy_eth_socket *eth_sk, struct rte_ether_hdr *eh,
              rte_be32_t ether_type);

int eth_send(xy_eth_socket *eth_sk, struct rte_mbuf *m_buf,
             struct rte_ether_hdr *eh, rte_be32_t ether_type);

#endif  // DPDK_TCP_INCLUDE_XY_ETH_H_
