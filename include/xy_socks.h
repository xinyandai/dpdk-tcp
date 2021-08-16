//
// Created by xydai on 2021/8/20.
//

#ifndef DPDK_TCP_INCLUDE_XY_SOCKS_H_
#define DPDK_TCP_INCLUDE_XY_SOCKS_H_
xy_tcp_socket *tcp_sock_lookup(xy_tcp_socket *listener,
                               rte_be32_t src_ip_be,
                               rte_be16_t src_port) ;

void syn_recv_tcp_sock_enqueue(xy_tcp_socket *tcp_sock);

void established_tcp_sock_enqueue(xy_tcp_socket *tcp_sock);

void syn_recv_tcp_sock_dequeue(xy_tcp_socket *tcp_sock);

void established_tcp_sock_dequeue(xy_tcp_socket *tcp_sock);

xy_tcp_socket *tcp_listener_lookup(rte_be32_t dst_ip_be,
                                   rte_be16_t dst_port);

#endif //DPDK_TCP_INCLUDE_XY_SOCKS_H_
