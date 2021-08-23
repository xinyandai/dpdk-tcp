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
                     "cannot listen when TCP_CLOSE");

  tcp_sk->state = TCP_LISTEN;
  listener_tcp_sock_enqueue(tcp_sk);

  return 0;
}

tcp_sock_t xy_accept(tcp_sock_t tcp_sk, uint32_t *ip, uint16_t *port) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, NULL,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, NULL,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_LISTEN, EINVAL, NULL,
                     "cannot listen when state is not TCP_LISTEN");

  return take_next_established();
}

ssize_t xy_recv(tcp_sock_t tcp_sk, char *buf, size_t len, int flags) {
  return 0;
}

ssize_t xy_send(tcp_sock_t tcp_sk, const char *buf, size_t len, int flags) {
  return 0;
}

int xy_close(tcp_sock_t tcp_sk) {
  return 0;
}
