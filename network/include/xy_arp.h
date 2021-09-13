//
// Created by xydai on 2021/8/20.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_ARP_H_
#define DPDK_TCP_INCLUDE_XY_ARP_H_

/**
 *
 * \param m_buf received rte_mbuf
 * \param eh ether header
 * \param len the length of the segment (including ether header).
 * \return 0 if handled, 1 if failed to reply when the transmit ring is full or
 * has been filled up.
 */
int arp_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh, int len);

#endif  // DPDK_TCP_INCLUDE_XY_ARP_H_
