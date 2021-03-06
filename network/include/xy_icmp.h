//
// Created by xydai on 2021/8/20.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_ICMP_H_
#define DPDK_TCP_INCLUDE_XY_ICMP_H_

#include <rte_mbuf.h>

#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>

/**
 *
 * \param m_buf
 * \param eh
 * \param iph
 * \param ipv4_hdr_len
 * \param len the length of the segment (including ether header).
 * \return 0 if success, 1 if failed to reply
 */
int icmp_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh,
              struct rte_ipv4_hdr *iph, int ipv4_hdr_len, int len);
#endif  // DPDK_TCP_INCLUDE_XY_ICMP_H_
