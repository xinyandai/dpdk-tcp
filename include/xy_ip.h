//
// Created by xydai on 2021/8/20.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_IP_H_
#define DPDK_TCP_INCLUDE_XY_IP_H_
#include <rte_mbuf.h>

#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>

#include "xy_struct.h"

/**
 * \brief process the received IP package.
 * \param m_buf the received rte_mbuf
 * \param eh the ether header lies in m_buf
 * \param len the length of the segment (including ether header).
 * \return 0 if success, 1 on error
 */
int ip_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh, int len);

/**
 * \brief setup the fields of rte_ipv4_hdr with next_proto_id, total_length, and
 * other information in ip_sk.
 * \param ip_sk
 * \param iph the pointer to ipv4 header
 * \param next_proto_id
 * \param total_length including ipv4 header and data
 * \return 0 if success, 1 on error
 */
int ip_setup(xy_ip_socket *ip_sk, struct rte_ipv4_hdr *iph,
             uint8_t next_proto_id, rte_be16_t total_length);

/**
 * \brief calculate the total_length by summing ipv4-header-len and data_len.
 * call the ip_setup to setup fields in ipv4-header
 * \return 0 if success, 1 on error
 */
int ip_send(xy_ip_socket *ip_sk, struct rte_mbuf *m_buf,
            struct rte_ipv4_hdr *iph, struct rte_ether_hdr *eh,
            uint8_t next_proto_id, uint16_t data_len);

/**
 * \brief call the ip_setup to setup fields in ipv4-header. No need to calculate
 * the total_length, since the total_length is reusable in forward case.
 *
 * \return 0 if success, 1 on error
 */
int ip_forward(xy_ip_socket *ip_sk, struct rte_mbuf *m_buf,
               struct rte_ipv4_hdr *iph, struct rte_ether_hdr *eh,
               uint8_t next_proto_id);

#endif  // DPDK_TCP_INCLUDE_XY_IP_H_
