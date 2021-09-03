#include <shared_mutex>
#include <thread>
#include "xy_api.h"

static XY_LIST_HEAD(list_established);

static std::shared_mutex shared_mtx;
#define read_lock() std::shared_lock<std::shared_mutex> lock(shared_mtx)
#define write_lock() std::unique_lock<std::shared_mutex> lock(shared_mtx)

inline void established_send_buffers() {
  read_lock();
  xy_list_node *head = &list_established;
  for (xy_list_node *pos = (head)->next; pos != (head); pos = pos->next) {
    tcp_send_buf((xy_tcp_socket *)pos);
  }
}

inline xy_tcp_socket *established_take_next() {
  static xy_list_node *cur = &list_established;
  static std::mutex cur_mtx;
  std::lock_guard<std::mutex> lk(cur_mtx);
  while (true) {
    {
      read_lock();
      if (cur->prev != &list_established) {
        cur = cur->prev;
        return (xy_tcp_socket *)cur;
      }
    }
    std::this_thread::yield();
  }
}

inline xy_tcp_socket *established_retrieve(xy_tcp_socket *tcp_sk) {
  while (true) {
    {
      read_lock();
      if (tcp_sk->state == TCP_ESTABLISHED) {
        return tcp_sk;
      }
    }
    std::this_thread::yield();
  }
}

inline void established_tcp_sock_enqueue(xy_tcp_socket *tcp_sock) {
  write_lock();
  xy_list_add_head(&list_established, (xy_list_node *)tcp_sock);
}

inline void established_tcp_sock_dequeue(xy_tcp_socket *tcp_sock) {
  write_lock();
  xy_list_del((xy_list_node *)tcp_sock);
}

inline xy_tcp_socket *established_tcp_sock_look_up(rte_be32_t dst_ip_be,
                                                   rte_be16_t dst_port_be,
                                                   rte_be32_t src_ip_be,
                                                   rte_be16_t src_port_be) {
  read_lock();
  xy_list_node *head = &list_established;
  for (xy_list_node *pos = (head)->next; pos != (head); pos = pos->next) {
    auto node = (xy_tcp_socket *)pos;

    if (node->ip_socket.ip_dst == src_ip_be && node->port_dst == src_port_be &&
        node->ip_socket.ip_src == dst_ip_be && node->port_src && dst_port_be) {
      return (xy_tcp_socket *)node;
    }
  }
}
