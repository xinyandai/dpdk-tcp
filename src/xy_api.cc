#include "xy_api.h"
#include "xy_api_asyn.h"

tcp_sock_t xy_socket(int domain, int type, int protocol) {

  xy_socket_ops socket_ops = {
      .type = XY_OPS_CREATE,
      .create_ = { tcp_sk }
  };
  xy_asyn_event_enqueue(&socket_ops);

  // init tcp_sk
  return socket_ops -> tcp_sk;
}

int xy_bind(tcp_sock_t tcp_sk, uint32_t ip, uint16_t port) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, -1,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, -1,
                     "the tcp socket id is invalid");
  xy_return_errno_if(port < 0 || port >= 65536 - 1, EINVAL, -1,
                     "invalid port number");

  xy_socket_ops socket_ops = {
      .type = XY_OPS_BIND,
      .bind_ = { tcp_sk, ip, port }
  };
  xy_asyn_event_enqueue(&socket_ops);

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

  xy_socket_ops socket_ops = {
      .type = XY_OPS_LISTEN,
      .bind_ = { tcp_sk }
  };
  xy_asyn_event_enqueue(&socket_ops);

  return 0;
}

tcp_sock_t xy_connect(tcp_sock_t tcp_sk, uint32_t *ip, uint16_t *port) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, NULL,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, NULL,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_CLOSE, EINVAL, NULL,
                     "cannot connect when state is not TCP_CLOSE");

  xy_socket_ops socket_ops = {
      .type = XY_OPS_CONNECT,
      .connect_ = { tcp_sk, ip, port }
  };
  xy_asyn_event_enqueue(&socket_ops);


  return established_retrieve(tcp_sk);
}

tcp_sock_t xy_accept(tcp_sock_t tcp_sk, uint32_t *ip, uint16_t *port) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, NULL,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, NULL,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_LISTEN, EINVAL, NULL,
                     "cannot accept when state is not TCP_LISTEN");
  xy_socket_ops socket_ops = {
      .type = XY_OPS_ACCEPT,
      .connect_ = { tcp_sk, ip, port }
  };
  xy_asyn_event_enqueue(&socket_ops);
  return established_take_next();
}

ssize_t xy_recv(tcp_sock_t tcp_sk, char *buf, size_t len, int flags) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, NULL,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, NULL,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_ESTABLISHED, EINVAL, NULL,
                     "cannot receive when state is not TCP_ESTABLISHED");
  xy_return_errno_if(tcp_sk->tcb == NULL, EINVAL, NULL,
                     "cannot receive when tcb is NULL");
  xy_buf_cursor *cur = &tcp_sk->tcb->read_cursor;

  ssize_t read = 0;
  while (true) {
    if (cur->buf == NULL || cur->left == 0) {
      struct rte_mbuf *m_buf = tcp_recv_dequeue(tcp_sk);

      if (!m_buf) {
        return read;
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
      return read;
    } else {
      rte_memcpy(buf, cur->buf, cur->left);
      read += cur->left;
      buf += cur->left;
      cur->buf = NULL;
      cur->left = 0;
      rte_pktmbuf_free(cur->mbuf);
    }
  }
}

ssize_t xy_send(tcp_sock_t tcp_sk, const char *buf, size_t len, int flags) {
  while (len > 0) {
    struct rte_mbuf *m_buf = rte_pktmbuf_alloc(buf_pool);
    rte_pktmbuf_append(m_buf, RTE_ETHER_MTU);
    xy_return_errno_if(m_buf == NULL, EBADF, 0,
                       "Failed to append MTU space in m_buf");
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
    tcp_send_enqueue(tcp_sk, m_buf);
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

  xy_socket_ops socket_ops = {
      .type = XY_OPS_CLOSE,
      .connect_ = { tcp_sk }
  };
  xy_asyn_event_enqueue(&socket_ops);
  return 0;
}
