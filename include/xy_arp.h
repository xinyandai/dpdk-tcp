//
// Created by xydai on 2021/8/20.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_ARP_H_
#define DPDK_TCP_INCLUDE_XY_ARP_H_

int arp_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh,
             int len);

#endif //DPDK_TCP_INCLUDE_XY_ARP_H_
