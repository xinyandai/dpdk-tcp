//
// Created by xydai on 2021/8/30.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_LIST_MBUF_H_
#define DPDK_TCP_INCLUDE_XY_LIST_MBUF_H_
#include <mutex>
#include <atomic>
#include <rte_ether.h>
#include <rte_mbuf.h>

#include "xy_list.h"
#include "xy_macros.h"
#include "xy_mpmc.h"

/***
 * the {struct rte_mbuf} is erased and reused as {xy_mbuf_list}, a struct of
 * double linked list.
 */
typedef struct xy_mbuf_list_t {
  xy_list_node list_node;
  struct rte_mbuf *mbuf;
} xy_mbuf_list;

static_assert(sizeof(xy_mbuf_list) <= XY_ETH_HDR_LEN + XY_IP_HDR_LEN);

static inline void xy_mbuf_list_init(xy_mbuf_list *head) {
  xy_list_init((xy_list_node *)head);
}

static inline void xy_mbuf_list_add_tail(xy_mbuf_list *head,
                                         struct rte_mbuf *mbuf) {
  xy_mbuf_list *new_node = rte_pktmbuf_mtod(mbuf, xy_mbuf_list *);
  new_node->mbuf = mbuf;

  xy_list_add_tail((xy_list_node *)head, (xy_list_node *)new_node);
}

static inline struct rte_mbuf * xy_mbuf_list_take_head(xy_mbuf_list *head) {
  xy_list_node * entry = xy_list_take_head((xy_list_node *)head);
  return ((xy_mbuf_list*) entry)->mbuf;
}

//int tcp_send_enqueue(xy_tcp_socket *tcp_sk, struct rte_mbuf *m_buf) {
//  xy_mbuf_list_add_tail(&tcp_sk->tcb->snd_buf_list, m_buf);
//}
//
//struct rte_mbuf * tcp_send_dequeue(xy_tcp_socket *tcp_sk) {
//  return xy_mbuf_list_take_head(&tcp_sk->tcb->snd_buf_list);
//}
//
//struct rte_mbuf *tcp_recv_dequeue(xy_tcp_socket *tcp_sk) {
//  return xy_mbuf_list_take_head(&tcp_sk->tcb->rcv_buf_list);
//}
#endif  // DPDK_TCP_INCLUDE_XY_LIST_MBUF_H_
