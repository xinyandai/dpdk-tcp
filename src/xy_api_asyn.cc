#include "xy_api_asyn.h"
#include "xy_api.h"

static mpmc_list event_queue;

static inline void xy_asyn_socket(xy_ops_create *create) {
  xy_tcp_socket *tcp_sk = allocate_tcp_socket();
  tcp_sk->id = tcp_socket_id();
  tcp_sk->state = TCP_CLOSE;

  create->tcp_sk = tcp_sk;
}

static inline void xy_asyn_bind(xy_ops_bind *bind) {
  xy_tcp_socket *tcp_sk = bind->tcp_sk;

  int register_ret = port_register(bind->port);
  if (register_ret != 0) {
    tcp_sk->ip_socket.ip_src = 0;
    tcp_sk->port_src = 0;
  } else {
    tcp_sk->ip_socket.ip_src = bind->ip;
    tcp_sk->port_src = bind->port;
  }
}

static inline void xy_asyn_listen(xy_ops_listen *listen) {
  listen->tcp_sk->state = TCP_LISTEN;
  listener_tcp_sock_enqueue(listen->tcp_sk);
}

static inline void xy_asyn_connect(xy_ops_connect *connect) {
  struct rte_mbuf *m_buf = rte_pktmbuf_alloc(buf_pool);
  struct rte_ether_hdr *eh = rte_pktmbuf_mtod(m_buf, struct rte_ether_hdr *);
  auto iph = (struct rte_ipv4_hdr *)((unsigned char *)(eh) + RTE_ETHER_HDR_LEN);
  auto tcp_h = (struct rte_tcp_hdr *)((unsigned char *)(iph) + XY_IP_HDR_LEN);

  uint16_t ip_len = XY_TCP_HDR_LEN + XY_IP_HDR_LEN;

  xy_tcp_socket *tcp_sk = connect->tcp_sk;

  eth_setup(&tcp_sk->ip_socket.eth_socket, eh,
            rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4));
  ip_setup(&tcp_sk->ip_socket, iph, IPPROTO_TCP, rte_cpu_to_be_16(ip_len));
  tcp_setup(tcp_sk, tcp_h, RTE_TCP_SYN_FLAG);
  tcp_send_enqueue(tcp_sk, m_buf);
}

static inline void xy_asyn_accept(xy_ops_accept *accept) {}

static inline int xy_asyn_close(xy_ops_close *close) {
  struct rte_mbuf *m_buf = rte_pktmbuf_alloc(buf_pool);
  struct rte_ether_hdr *eh = rte_pktmbuf_mtod(m_buf, struct rte_ether_hdr *);
  auto iph = (struct rte_ipv4_hdr *)((unsigned char *)(eh) + RTE_ETHER_HDR_LEN);
  auto tcp_h = (struct rte_tcp_hdr *)((unsigned char *)(iph) + XY_IP_HDR_LEN);

  uint16_t ip_len = XY_TCP_HDR_LEN + XY_IP_HDR_LEN;

  xy_tcp_socket *tcp_sk = close->tcp_sk;
  eth_setup(&tcp_sk->ip_socket.eth_socket, eh,
            rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4));
  ip_setup(&tcp_sk->ip_socket, iph, IPPROTO_TCP, rte_cpu_to_be_16(ip_len));
  tcp_setup(tcp_sk, tcp_h, RTE_TCP_FIN_FLAG);

  tcp_sk->state = TCP_FIN_WAIT_1;

  tcp_send_enqueue(tcp_sk, m_buf);
  return 0;
}

static inline xy_asyn_event_handle_ops(xy_socket_ops* ops) {

  switch (ops->type) {
      case XY_OPS_CREATE:
        xy_asyn_socket(&ops->create_);
        break;
      case XY_OPS_BIND:
        xy_asyn_bind(&ops->bind_);
        break;
      case XY_OPS_LISTEN:
        xy_asyn_listen(&ops->listen_);
        break;
      case XY_OPS_CONNECT:
        xy_asyn_connect(&ops->connect_);
        break;
      case XY_OPS_CLOSE:
        xy_asyn_close(&ops->close_);
        break;
      case XY_OPS_ACCEPT:
        xy_asyn_accept(&ops->accept_);
        break;
    }
  {
    std::lock_guard<std::mutex> lock(mutex);
    ops->done = 1;
  }
  ops->cv.notify_one();
}

void xy_asyn_event_handle() {
  mpmc_list_node *node;
  while ((node = mpmc_list_del_head(&event_queue.head, &event_queue.tail))) {
    xy_asyn_event_handle_ops((xy_socket_ops *)node);
  }
}

void xy_asyn_event_enqueue(xy_socket_ops *ops) {
  mpmc_list_add_tail(&event_queue.tail, (mpmc_list_node*) ops);
  
  std::unique_lock<std::mutex> lock(ops->mutex));
  ops->cv.wait(lock, [] { return ops->done; });
}
