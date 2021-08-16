#pragma once
#ifndef __XY_TCP_H__
#define __XY_TCP_H__

#include <rte_mbuf.h>

#include <rte_ip.h>
#include <rte_tcp.h>
#include <rte_ether.h>

#include "xy_ip.h"
#include "xy_eth.h"
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

//                       TCP Connection State Diagram

int port_register(uint16_t port);
xy_tcp_socket *allocate_tcp_socket();


int tcp_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh,
             struct rte_ipv4_hdr *iph, int ipv4_hdr_len, int len);

int tcp_send(xy_tcp_socket *tcp_sk, struct rte_mbuf *m_buf,
             struct rte_tcp_hdr *tcp_h, struct rte_ipv4_hdr *iph,
             struct rte_ether_hdr *eh, uint8_t tcp_flags,
             rte_be32_t sent_seq, rte_be32_t recv_ack);
/**
struct deleted_tcp_sock {
  struct sock sk;
  int fd;
  uint16_t tcp_header_len;
  struct tcb tcb;
  uint8_t flags;
  uint8_t backoff;
  int32_t srtt;
  int32_t rttvar;
  uint32_t rto;
  struct timer *retransmit;
  struct timer *delack;
  struct timer *keepalive;
  struct timer *linger;
  uint8_t delacks;
  uint16_t rmss;
  uint16_t smss;  // SENDER MAXIMUM SEGMENT SIZE (SMSS): The size does not
                  // include the TCP/IP headers and options.
  uint16_t cwnd;  // RECEIVER MAXIMUM SEGMENT SIZE (RMSS)
  uint32_t inflight;

  uint8_t sackok;
  uint8_t sacks_allowed;
  uint8_t sacklen;
  struct tcp_sack_block sacks[4];

  uint8_t tsopt;

  struct sk_buff_head ofo_queue; // Out-of-order queue
}
*/
#endif