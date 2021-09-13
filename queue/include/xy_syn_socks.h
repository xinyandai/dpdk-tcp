//
// Created by xydai on 2021/9/1.
//

#ifndef DPDK_TCP_MIDDLEWARE_SY_SYN_SOCKS_H_
#define DPDK_TCP_MIDDLEWARE_SY_SYN_SOCKS_H_

void established_send_buffers();

xy_tcp_socket *established_take_next();

xy_tcp_socket *established_retrieve(xy_tcp_socket *tcp_sk);

void established_tcp_sock_enqueue(xy_tcp_socket *tcp_sock);

void established_tcp_sock_dequeue(xy_tcp_socket *tcp_sock);

xy_tcp_socket *established_tcp_sock_look_up(rte_be32_t dst_ip_be,
                                            rte_be16_t dst_port_be,
                                            rte_be32_t src_ip_be,
                                            rte_be16_t src_port_be);
#endif  // DPDK_TCP_MIDDLEWARE_SY_SYN_SOCKS_H_
