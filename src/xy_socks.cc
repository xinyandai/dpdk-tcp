#include "xy_api.h"

xy_tcp_socket *tcp_sock_lookup(xy_tcp_socket *listener,
                               rte_be32_t src_ip_be,
                               rte_be16_t src_port) {
  uint32_t src_ip = rte_be_to_cpu_32(src_ip_be);
  // TODO
}

void syn_recv_tcp_sock_enqueue(xy_tcp_socket *tcp_sock) {
  // TODO
}

void established_tcp_sock_enqueue(xy_tcp_socket *tcp_sock) {
  // TODO
}

void syn_recv_tcp_sock_dequeue(xy_tcp_socket *tcp_sock) {
  // TODO
}

void established_tcp_sock_dequeue(xy_tcp_socket *tcp_sock) {
  // TODO
}

xy_tcp_socket *tcp_listener_lookup(rte_be32_t dst_ip_be,
                                   rte_be16_t dst_port) {
  // TODO
}