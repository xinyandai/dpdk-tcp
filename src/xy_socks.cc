#include "xy_api.h"

xy_tcp_socket *allocate_tcp_socket() {
  return (xy_tcp_socket *)malloc(sizeof(xy_tcp_socket));
}


static LIST_HEAD(list_listeners);
static LIST_HEAD(list_syn_recved);
static LIST_HEAD(list_established);

void syn_recv_tcp_sock_enqueue(xy_tcp_socket *tcp_sock) {
  list_add_head(&list_syn_recved, (list_head *)tcp_sock);
}

void syn_recv_tcp_sock_dequeue(xy_tcp_socket *tcp_sock) {
  list_del((list_head *)tcp_sock);
}

void established_tcp_sock_enqueue(xy_tcp_socket *tcp_sock) {
  list_add_head(&list_established, (list_head *)tcp_sock);
}

void established_tcp_sock_dequeue(xy_tcp_socket *tcp_sock) {
  list_del((list_head *)tcp_sock);
}

void listener_tcp_sock_enqueue(xy_tcp_socket *tcp_sock) {
  list_add_head(&list_listeners, (list_head *)tcp_sock);
}

void listener_tcp_sock_dequeue(xy_tcp_socket *tcp_sock) {
  list_del((list_head *)tcp_sock);
}

xy_tcp_socket *tcp_listener_lookup(rte_be32_t dst_ip_be, rte_be16_t dst_port) {
  list_head *head = &list_listeners;
  for (list_head *pos = (head)->next; pos != (head); pos = pos->next) {
    xy_tcp_socket *node = (xy_tcp_socket *)pos;

    if (node->ip_socket.ip_src == dst_ip_be && node->port_src == dst_port) {
      assert(node->state == TCP_LISTEN);
      return (xy_tcp_socket *)node;
    }
  }
  return NULL;
}

static xy_tcp_socket *tcp_sock_list_lookup_from_head(list_head *socket_head,
                                                     rte_be32_t dst_ip_be,
                                                     rte_be16_t dst_port,
                                                     rte_be32_t src_ip,
                                                     rte_be16_t src_port) {
  list_head *head = (list_head *)socket_head;
  for (list_head *pos = (head)->next; pos != (head); pos = pos->next) {
    xy_tcp_socket *node = (xy_tcp_socket *)pos;

    if (node->ip_socket.ip_dst == src_ip && node->port_dst == src_port &&
        node->ip_socket.ip_src == dst_ip_be && node->port_src && dst_port) {
      return (xy_tcp_socket *)node;
    }
  }
}

xy_tcp_socket *tcp_sock_lookup(rte_be32_t dst_ip_be, rte_be16_t dst_port,
                               rte_be32_t src_ip, rte_be16_t src_port) {
  xy_tcp_socket *find_established = tcp_sock_list_lookup_from_head(
      &list_established, dst_ip_be, dst_port, src_ip, src_port);
  if (find_established) {
    return find_established;
  }

  xy_tcp_socket *find_syn_recv = tcp_sock_list_lookup_from_head(
      &list_syn_recved, dst_ip_be, dst_port, src_ip, src_port);
  return find_syn_recv;
}
