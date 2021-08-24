#include "xy_api.h"

tcp_sock_t xy_socket(int domain, int type, int protocol) {
  xy_return_errno_if(domain != AF_INET, EAFNOSUPPORT, NULL,
                     "domain should be AF_INET");
  xy_return_errno_if(type != SOCK_STREAM, EINVAL, NULL,
                     "type should be SOCK_STREAM");
  xy_return_errno_if(protocol != IPPROTO_TCP, EINVAL, NULL,
                     "protocol should be IPPROTO_TCP");

  xy_tcp_socket *tcp_sk = allocate_tcp_socket();
  tcp_sk->id = tcp_socket_id();
  tcp_sk->state = TCP_CLOSE;

  xy_return_errno_if(NULL == tcp_sk, ENFILE, NULL, "bad alloc");
  // init tcp_sk
  return tcp_sk;
}

int xy_bind(tcp_sock_t tcp_sk, uint32_t ip, uint16_t port) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, -1,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, -1,
                     "the tcp socket id is invalid");
  xy_return_errno_if(port < 0 || port >= 65536 - 1, EINVAL, -1,
                     "invalid port number");

  int register_ret = port_register(port);
  xy_return_errno_if(register_ret != 0, EBADF, register_ret,
                     "port is registered");

  tcp_sk->ip_socket.ip_src = ip;
  tcp_sk->port_src = port;

  return 0;
}

int xy_listen(tcp_sock_t tcp_sk, int backlog) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, -1,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, -1,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_CLOSE, EINVAL, -1,
                     "cannot listen when state is not TCP_CLOSE");

  tcp_sk->state = TCP_LISTEN;
  listener_tcp_sock_enqueue(tcp_sk);

  return 0;
}

tcp_sock_t connect(tcp_sock_t tcp_sk, uint32_t *ip, uint16_t *port) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, NULL,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, NULL,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_CLOSE, EINVAL, NULL,
                     "cannot connect when state is not TCP_CLOSE");
  struct rte_mbuf *m_buf = rte_pktmbuf_alloc(buf_pool);
  struct rte_ether_hdr *eh = rte_pktmbuf_mtod(m_buf, struct rte_ether_hdr *);
  auto iph = (struct rte_ipv4_hdr *)((unsigned char *)(eh) + RTE_ETHER_HDR_LEN);
  auto tcp_h = (struct rte_tcp_hdr *)((unsigned char *)(iph) + XY_IP_HDR_LEN);

  uint16_t ip_len = XY_TCP_HDR_LEN + XY_IP_HDR_LEN;

  eth_setup(&tcp_sk->ip_socket.eth_socket, eh,
            rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4));
  ip_setup(&tcp_sk->ip_socket, iph, IPPROTO_TCP, rte_cpu_to_be_16(ip_len));
  tcp_setup(tcp_sk, tcp_h, RTE_TCP_SYN_FLAG);
  send_enqueue(tcp_sk, m_buf);

  return established_retrieve(tcp_sk);
}

tcp_sock_t xy_accept(tcp_sock_t tcp_sk, uint32_t *ip, uint16_t *port) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, NULL,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, NULL,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_LISTEN, EINVAL, NULL,
                     "cannot accept when state is not TCP_LISTEN");

  return established_take_next();
}

typedef struct {
  const char *buf;
  uint16_t left;
} xy_buf_cursor;

ssize_t xy_recv(tcp_sock_t tcp_sk, char *buf, size_t len, int flags) {
  static xy_buf_cursor cur = {.buf = NULL, .left = 0};

  size_t read = 0;
  while (true) {
    if (cur.buf == NULL || cur.left == 0) {
      struct rte_mbuf *m_buf = recv_dequeue(tcp_sk);

      if (!m_buf) {
        return read;
      }

      /// reset cursor
      struct rte_ether_hdr *eh =
          rte_pktmbuf_mtod(m_buf, struct rte_ether_hdr *);
      auto iph =
          (struct rte_ipv4_hdr *)((unsigned char *)(eh) + RTE_ETHER_HDR_LEN);
      uint8_t iph_len = rte_ipv4_hdr_len(iph);
      auto tcp_hdr_len = sizeof(struct rte_tcp_hdr);
      auto tcp_h = (struct rte_tcp_hdr *)((unsigned char *)(iph) + iph_len);

      cur.buf = (char *)tcp_h + tcp_hdr_len;
      cur.left = iph->total_length - iph_len - tcp_hdr_len;
    }

    if (cur.left >= len) {
      rte_memcpy(buf, cur.buf, len);
      read += len;
      buf += len;
      cur.buf += len;
      cur.left -= len;
      return read;
    } else {
      rte_memcpy(buf, cur.buf, cur.left);
      read += cur.left;
      buf += cur.left;
      cur.buf = NULL;
      cur.left = 0;
    }
  }
}

ssize_t xy_send(tcp_sock_t tcp_sk, const char *buf, size_t len, int flags) {
  while (len > 0) {
    struct rte_mbuf *m_buf = rte_pktmbuf_alloc(buf_pool);
    struct rte_ether_hdr *eh = rte_pktmbuf_mtod(m_buf, struct rte_ether_hdr *);
    auto iph =
        (struct rte_ipv4_hdr *)((unsigned char *)(eh) + RTE_ETHER_HDR_LEN);
    auto tcp_h = (struct rte_tcp_hdr *)((unsigned char *)(iph) + XY_IP_HDR_LEN);
    char *tcp_data = (char *)((unsigned char *)(iph) + XY_TCP_HDR_LEN);

    uint16_t data_len = xy_min(XY_TCP_MAX_DATA_LEN, len);
    uint16_t ip_len = data_len + XY_TCP_HDR_LEN + XY_IP_HDR_LEN;
    rte_memcpy(tcp_data, buf, data_len);

    eth_setup(&tcp_sk->ip_socket.eth_socket, eh,
              rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4));
    ip_setup(&tcp_sk->ip_socket, iph, IPPROTO_TCP, rte_cpu_to_be_16(ip_len));
    tcp_setup(tcp_sk, tcp_h, 0);

    len -= data_len;
    buf += data_len;
    send_enqueue(tcp_sk, m_buf);
  }
  return 0;
}

int xy_close(tcp_sock_t tcp_sk) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, NULL,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, NULL,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_ESTABLISHED, EINVAL, NULL,
                     "cannot connect when state is not TCP_ESTABLISHED");

  struct rte_mbuf *m_buf = rte_pktmbuf_alloc(buf_pool);
  struct rte_ether_hdr *eh = rte_pktmbuf_mtod(m_buf, struct rte_ether_hdr *);
  auto iph = (struct rte_ipv4_hdr *)((unsigned char *)(eh) + RTE_ETHER_HDR_LEN);
  auto tcp_h = (struct rte_tcp_hdr *)((unsigned char *)(iph) + XY_IP_HDR_LEN);

  uint16_t ip_len = XY_TCP_HDR_LEN + XY_IP_HDR_LEN;

  eth_setup(&tcp_sk->ip_socket.eth_socket, eh,
            rte_cpu_to_be_16(RTE_ETHER_TYPE_IPV4));
  ip_setup(&tcp_sk->ip_socket, iph, IPPROTO_TCP, rte_cpu_to_be_16(ip_len));
  tcp_setup(tcp_sk, tcp_h, RTE_TCP_FIN_FLAG);

  send_enqueue(tcp_sk, m_buf);
  return 0;
}
