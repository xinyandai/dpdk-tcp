#pragma once
#ifndef ____DPDK_TCP_INCLUDE_XY_TCP_H__
#define ____DPDK_TCP_INCLUDE_XY_TCP_H__

#include <rte_mbuf.h>

#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>

#include "xy_struct.h"

//                                            Transmission Control Protocol
//                                                 Functional Specification
//
//
//
//
//                               +---------+ ---------\      active OPEN
//                               |  CLOSED |            \    -----------
//                               +---------+<---------\   \   create TCB
//                                 |     ^              \   \  snd SYN
//                    passive OPEN |     |   CLOSE        \   \
//                    ------------ |     | ----------       \   \
//                     create TCB  |     | delete TCB         \   \
//                                 V     |                      \   \
//                               +---------+            CLOSE    |    \
//                               |  LISTEN |          ---------- |     |
//                               +---------+          delete TCB |     |
//                    rcv SYN      |     |     SEND              |     |
//                   -----------   |     |    -------            |     V
//  +---------+      snd SYN,ACK  /       \   snd SYN          +---------+
//  |         |<-----------------           ------------------>|         |
//  |   SYN   |                    rcv SYN                     |   SYN   |
//  |   RCVD  |<-----------------------------------------------|   SENT  |
//  |         |                    snd ACK                     |         |
//  |         |------------------           -------------------|         |
//  +---------+   rcv ACK of SYN  \       /  rcv SYN,ACK       +---------+
//    |           --------------   |     |   -----------
//    |                  x         |     |     snd ACK
//    |                            V     V
//    |  CLOSE                   +---------+
//    | -------                  |  ESTAB  |
//    | snd FIN                  +---------+
//    |                   CLOSE    |     |    rcv FIN
//    V                  -------   |     |    -------
//  +---------+          snd FIN  /       \   snd ACK          +---------+
//  |  FIN    |<-----------------           ------------------>|  CLOSE  |
//  | WAIT-1  |------------------                              |   WAIT  |
//  +---------+          rcv FIN  \                            +---------+
//    | rcv ACK of FIN   -------   |                            CLOSE  |
//    | --------------   snd ACK   |                           ------- |
//    V        x                   V                           snd FIN V
//  +---------+                  +---------+                   +---------+
//  |FINWAIT-2|                  | CLOSING |                   | LAST-ACK|
//  +---------+                  +---------+                   +---------+
//    |                rcv ACK of FIN |                 rcv ACK of FIN |
//    |  rcv FIN       -------------- |    Timeout=2MSL -------------- |
//    |  -------              x       V    ------------        x       V
//     \ snd ACK                 +---------+delete TCB         +---------+
//      ------------------------>|TIME WAIT|------------------>| CLOSED  |
//                               +---------+                   +---------+
//
//                       TCP Connection State Diagram

int tcp_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh,
             struct rte_ipv4_hdr *iph, int ipv4_hdr_len, int len);

int tcp_setup(xy_tcp_socket *tcp_sk, struct rte_tcp_hdr *tcp_h,
              uint8_t tcp_flags);

int tcp_ready(struct rte_tcp_hdr *tcp_h, struct rte_ipv4_hdr *iph,
              uint32_t sent_seq, uint32_t recv_ack);

int tcp_send(xy_tcp_socket *tcp_sk, struct rte_mbuf *m_buf, uint8_t tcp_flags,
             rte_be32_t sent_seq, rte_be32_t recv_ack, uint16_t data_len);

int tcp_send(xy_tcp_socket *tcp_sk, struct rte_mbuf *m_buf,
             struct rte_tcp_hdr *tcp_h, struct rte_ipv4_hdr *iph,
             struct rte_ether_hdr *eh, uint8_t tcp_flags, rte_be32_t sent_seq,
             rte_be32_t recv_ack, uint16_t data_len);

int tcp_forward(xy_tcp_socket *tcp_sk, struct rte_mbuf *m_buf,
                struct rte_tcp_hdr *tcp_h, struct rte_ipv4_hdr *iph,
                struct rte_ether_hdr *eh, uint8_t tcp_flags,
                rte_be32_t sent_seq, rte_be32_t recv_ack);

int tcp_send_buf(xy_tcp_socket *tcp_sk);


#endif