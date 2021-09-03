#include "xy_syn_api.h"
#include <cstring>
#include "xy_api.h"

static mpmc_list event_queue;

static inline void xy_syn_socket(xy_ops_create *create) {
  xy_tcp_socket *tcp_sk = allocate_tcp_socket();
  std::memset(tcp_sk, 0, sizeof(xy_tcp_socket));

  tcp_sk->id = tcp_socket_id();
  tcp_sk->ref_cnt = 1;
  tcp_sk->state = TCP_CLOSE;
  tcp_sk->tcb = allocate_tcb();

  create->tcp_sk = tcp_sk;
}

static inline void xy_syn_bind(xy_ops_bind *bind) {
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

static inline void xy_syn_listen(xy_ops_listen *listen) {
  listen->tcp_sk->state = TCP_LISTEN;
  listener_tcp_sock_enqueue(listen->tcp_sk);
}

static inline void xy_syn_connect(xy_ops_connect *connect) {
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

  xy_mbuf_list_add_tail(&tcp_sk->tcb->snd_buf_list, m_buf);
}

static inline void xy_syn_accept(xy_ops_accept *accept) {}

static inline int xy_syn_close(xy_ops_close *close) {
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

  switch (tcp_sk->state) {
    case TCP_ESTABLISHED:
      established_tcp_sock_dequeue(tcp_sk);
      break;
    case TCP_SYN_RECEIVED:
      syn_recv_tcp_sock_dequeue(tcp_sk);
      break;
  }

  tcp_sk->state = TCP_FIN_WAIT_1;

  xy_mbuf_list_add_tail(&tcp_sk->tcb->snd_buf_list, m_buf);
  return 0;
}

static inline void xy_syn_send(xy_ops_send *send) {
  xy_tcp_socket *tcp_sk = send->tcp_sk;
  const char *buf = send->buf;
  size_t len = send->len;
  ssize_t sent = 0;
  while (len > 0) {
    struct rte_mbuf *m_buf = rte_pktmbuf_alloc(buf_pool);
    rte_pktmbuf_append(m_buf, RTE_ETHER_MTU);
    if (m_buf == NULL) {
      break;
    }
    struct rte_ether_hdr *eh = rte_pktmbuf_mtod(m_buf, struct rte_ether_hdr *);
    auto iph =
        (struct rte_ipv4_hdr *)((unsigned char *)(eh) + RTE_ETHER_HDR_LEN);
    auto tcp_h = (struct rte_tcp_hdr *)((unsigned char *)(iph) + XY_IP_HDR_LEN);
    char *tcp_data = (char *)((unsigned char *)(iph) + XY_TCP_HDR_LEN);

    uint16_t data_len = xy_min(XY_TCP_MAX_DATA_LEN, len);
    uint16_t ip_len = data_len + XY_TCP_HDR_LEN + XY_IP_HDR_LEN;
    rte_memcpy(tcp_data, buf, data_len);

    m_buf->pkt_len = m_buf->data_len =
        ip_len + XY_IP_HDR_LEN + RTE_ETHER_HDR_LEN;

    eth_setup(&tcp_sk->ip_socket.eth_socket, eh,
              rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4));
    ip_setup(&tcp_sk->ip_socket, iph, IPPROTO_TCP, rte_cpu_to_be_16(ip_len));
    tcp_setup(tcp_sk, tcp_h, 0);

    len -= data_len;
    buf += data_len;
    sent += data_len;
    xy_mbuf_list_add_tail(&tcp_sk->tcb->snd_buf_list, m_buf);
  }
  send->ret = sent;
}
static inline void xy_syn_recv(xy_ops_recv *recv) {
  xy_tcp_socket *tcp_sk = recv->tcp_sk;
  char *buf = recv->buf;
  size_t len = recv->len;

  xy_buf_cursor *cur = &tcp_sk->tcb->read_cursor;

  ssize_t read = 0;
  while (true) {
    if (cur->buf == NULL || cur->left == 0) {
      struct rte_mbuf *m_buf =
          xy_mbuf_list_take_head(&tcp_sk->tcb->snd_buf_list);

      if (!m_buf) {
        break;
      }

      /// reset cursor
      auto eh = rte_pktmbuf_mtod(m_buf, struct rte_ether_hdr *);
      auto iph =
          (struct rte_ipv4_hdr *)((unsigned char *)(eh) + RTE_ETHER_HDR_LEN);
      uint8_t iph_len = rte_ipv4_hdr_len(iph);
      auto tcp_h = (struct rte_tcp_hdr *)((unsigned char *)(iph) + iph_len);

      cur->buf = (char *)tcp_h + XY_TCP_HDR_LEN;
      cur->left = iph->total_length - iph_len - XY_TCP_HDR_LEN;
    }

    if (cur->left >= len) {
      rte_memcpy(buf, cur->buf, len);
      read += len;
      // buf += len;
      cur->buf += len;
      cur->left -= len;
      break;
    } else {
      rte_memcpy(buf, cur->buf, cur->left);
      read += cur->left;
      buf += cur->left;
      cur->buf = NULL;
      cur->left = 0;
      rte_pktmbuf_free(cur->mbuf);
    }
  }

  recv->len = len;
  recv->buf = buf;
  recv->ret = read;
}

static inline void xy_syn_event_handle_ops(xy_socket_ops *ops) {
  switch (ops->type) {
    case XY_OPS_CREATE:
      xy_syn_socket(&ops->create_);
      break;
    case XY_OPS_BIND:
      xy_syn_bind(&ops->bind_);
      break;
    case XY_OPS_LISTEN:
      xy_syn_listen(&ops->listen_);
      break;
    case XY_OPS_CONNECT:
      xy_syn_connect(&ops->connect_);
      break;
    case XY_OPS_CLOSE:
      xy_syn_close(&ops->close_);
      break;
    case XY_OPS_ACCEPT:
      xy_syn_accept(&ops->accept_);
      break;
    case XY_OPS_SEND:
      xy_syn_send(&ops->send_);
      break;
    case XY_OPS_RECV:
      xy_syn_recv(&ops->recv_);
      break;
  }
  {
    std::lock_guard<std::mutex> lock(ops->mutex);
    ops->done = 1;
  }
  ops->cv.notify_one();
}

void xy_syn_event_handle() {
  mpmc_list_node *node;
  while ((node = mpmc_list_del_head(&event_queue.head, &event_queue.tail))) {
    xy_syn_event_handle_ops((xy_socket_ops *)node);
  }
}

void xy_syn_event_enqueue(xy_socket_ops *ops) {
  mpmc_list_add_tail(&event_queue.tail, (mpmc_list_node *)ops);

  std::unique_lock<std::mutex> lock(ops->mutex);
  ops->cv.wait(lock, [ops] { return ops->done; });
}
