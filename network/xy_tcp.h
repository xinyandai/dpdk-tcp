#pragma once
#ifndef ____DPDK_TCP_INCLUDE_XY_TCP_H__
#define ____DPDK_TCP_INCLUDE_XY_TCP_H__

#include <cstdlib>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_ip.h>
#include <rte_tcp.h>

#include "xy_struct.h"
/**
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
*/
/**
 * \brief process the received TCP package.
 * \param m_buf the received rte_mbuf package.
 * \param eh  eth header lies in m_buf.
 * \param iph ip header lies in m_buf.
 * \param ipv4_hdr_len the length of ipv4 header.
 * \param len the length of the segment (including ether header).
 * \return 0 if success, 1 on error
 */
int tcp_recv(struct rte_mbuf *m_buf, struct rte_ether_hdr *eh,
             struct rte_ipv4_hdr *iph, int ipv4_hdr_len, int len);

/**
 * \brief setup the fields (except sent_seq, recv_ack and cksum, which would be
 * filled in tcp_ready) of rte_tcp_hdr with tcp_flags, and other information in
 * tcp_sk.
 * \param tcp_sk
 * \param tcp_h
 * \param tcp_flags
 * \return 0 if success, 1 on error
 */
int tcp_setup(xy_tcp_socket *tcp_sk, struct rte_tcp_hdr *tcp_h,
              uint8_t tcp_flags);

/**
 * filled sent_seq and recv_ack in {iph}, then calculate the check sum.
 * \param tcp_h
 * \param iph
 * \param sent_seq
 * \param recv_ack
 * \return 0
 */
int tcp_ready(struct rte_tcp_hdr *tcp_h, struct rte_ipv4_hdr *iph,
              uint32_t sent_seq, uint32_t recv_ack);

/**
 * \brief first call tcp_setup() and tcp_ready() to filled up the fields and
 * calculate the checksum, then call ip_send() and eth_send() to send the
 * package. {data_len} field is required here to calculate IP total length.
 *
 * \param tcp_sk
 * \param m_buf
 * \param tcp_flags
 * \param sent_seq seq
 * \param recv_ack ack
 * \param data_len the length of tcp data
 * \return
 */
int tcp_send(xy_tcp_socket *tcp_sk, struct rte_mbuf *m_buf, uint8_t tcp_flags,
             rte_be32_t sent_seq, rte_be32_t recv_ack, uint16_t data_len);

/**
 * \brief first call tcp_setup() and tcp_ready() to filled up the fields and
 * calculate the checksum, then call ip_forward() and eth_send() to send the
 * package. {data_len} field is not needed here since the package length does
 * not change.
 *
 * \param tcp_sk
 * \param m_buf
 * \param tcp_h
 * \param iph
 * \param eh
 * \param tcp_flags
 * \param sent_seq
 * \param recv_ack
 * \return
 */
int tcp_forward(xy_tcp_socket *tcp_sk, struct rte_mbuf *m_buf,
                struct rte_tcp_hdr *tcp_h, struct rte_ipv4_hdr *iph,
                struct rte_ether_hdr *eh, uint8_t tcp_flags,
                rte_be32_t sent_seq, rte_be32_t recv_ack);

/**
 * \brief Fetch packages from tcp_sk->tcb->snd_buf_list to send according the
 * snd_wnd. Retransmit packages in tcp_sk->tcb->snd_wnd_buffer by timeout policy
 * or acknowledgement policy.
 *
 * \param tcp_sk
 * \return 0
 */
int tcp_send_buf(xy_tcp_socket *tcp_sk);


static inline uint32_t random_generate_iss() {
  return rand();
}
#endif