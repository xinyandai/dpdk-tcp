//
// Created by xydai on 2021/8/20.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_IP_H_
#define DPDK_TCP_INCLUDE_XY_IP_H_
#include <rte_mbuf.h>

#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_ether.h>

#include "xy_eth.h"
#include "xy_tcp.h"
#include "xy_struct.h"



int ip_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh,
            int len);

int ip_send(xy_ip_socket *ip_sk, struct rte_mbuf *m_buf,
            struct rte_ipv4_hdr *iph, struct rte_ether_hdr *eh,
            uint8_t next_proto_id);

#endif //DPDK_TCP_INCLUDE_XY_IP_H_
