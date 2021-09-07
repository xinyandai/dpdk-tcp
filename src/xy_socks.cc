#include <mutex>
#include "xy_api.h"

static XY_LIST_HEAD(list_listeners);
static XY_LIST_HEAD(list_syn_recved);

uint32_t tcp_socket_id() {
  static uint32_t id = 0;
  return ++id;
}

xy_tcp_socket *allocate_tcp_socket() {
  return (xy_tcp_socket *)malloc(sizeof(xy_tcp_socket));
}

struct tcb *allocate_tcb() {
  return (struct tcb *)malloc(sizeof(struct tcb));
}

void deallocate_tcb(xy_tcp_socket *tcp_sock) {
  free(tcp_sock->tcb);
  tcp_sock->tcb = NULL;
}

void deallocate_tcp_socket(xy_tcp_socket *tcp_sock) { free(tcp_sock); }

void syn_recv_tcp_sock_enqueue(xy_tcp_socket *tcp_sock) {
  xy_list_add_head(&list_syn_recved, (xy_list_node *)tcp_sock);
}

void syn_recv_tcp_sock_dequeue(xy_tcp_socket *tcp_sock) {
  xy_list_del((xy_list_node *)tcp_sock);
}

void listener_tcp_sock_enqueue(xy_tcp_socket *tcp_sock) {
  xy_list_add_head(&list_listeners, (xy_list_node *)tcp_sock);
}

void listener_tcp_sock_dequeue(xy_tcp_socket *tcp_sock) {
  xy_list_del((xy_list_node *)tcp_sock);
}

xy_tcp_socket *listener_tcp_sock_lookup(rte_be32_t dst_ip_be,
                                        rte_be16_t dst_port) {
  xy_list_node *head = &list_listeners;
  for (xy_list_node *pos = (head)->next; pos != (head); pos = pos->next) {
    xy_tcp_socket *node = (xy_tcp_socket *)pos;

    if (node->ip_socket.ip_src == dst_ip_be && node->port_src == dst_port) {
      assert(node->state == TCP_LISTEN);
      return (xy_tcp_socket *)node;
    }
  }
  return NULL;
}

xy_tcp_socket *syn_rcvd_tcp_sock_lookup(rte_be32_t dst_ip_be,
                                        rte_be16_t dst_port_be,
                                        rte_be32_t src_ip_be,
                                        rte_be16_t src_port_be) {
  xy_list_node *head = &list_syn_recved;
  for (xy_list_node *pos = (head)->next; pos != (head); pos = pos->next) {
    xy_tcp_socket *node = (xy_tcp_socket *)pos;

    if (node->ip_socket.ip_dst == src_ip_be && node->port_dst == src_port_be &&
        node->ip_socket.ip_src == dst_ip_be && node->port_src && dst_port_be) {
      return (xy_tcp_socket *)node;
    }
  }
  return NULL;
}
