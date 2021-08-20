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


int eth_recv(struct rte_mbuf *buf);

int eth_send(xy_eth_socket *eth_sk, struct rte_mbuf *m_buf,
             struct rte_ether_hdr *eh, uint16_t ether_type);

#endif  // DPDK_TCP_INCLUDE_XY_ETH_H_
