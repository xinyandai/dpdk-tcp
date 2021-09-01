#include "xy_api.h"
#include "xy_syn_api.h"

tcp_sock_t xy_socket(int domain, int type, int protocol) {
  xy_socket_ops socket_ops = {.type = XY_OPS_CREATE, .create_ = {NULL}};
  xy_syn_event_enqueue(&socket_ops);

  // init tcp_sk
  return socket_ops.create_.tcp_sk;
}

int xy_bind(tcp_sock_t tcp_sk, uint32_t ip, uint16_t port) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, -1,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, -1,
                     "the tcp socket id is invalid");
  xy_return_errno_if(port < 0 || port >= 65536 - 1, EINVAL, -1,
                     "invalid port number");

  xy_socket_ops socket_ops = {.type = XY_OPS_BIND, .bind_ = {tcp_sk, ip, port}};
  xy_syn_event_enqueue(&socket_ops);

  return 0;
}

int xy_listen(tcp_sock_t tcp_sk, int backlog) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, -1,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, -1,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_CLOSE, EINVAL, -1,
                     "cannot listen when state is not TCP_CLOSE");

  xy_socket_ops socket_ops = {.type = XY_OPS_LISTEN, .bind_ = {tcp_sk}};
  xy_syn_event_enqueue(&socket_ops);

  return 0;
}

tcp_sock_t xy_connect(tcp_sock_t tcp_sk, uint32_t *ip, uint16_t *port) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, NULL,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, NULL,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_CLOSE, EINVAL, NULL,
                     "cannot connect when state is not TCP_CLOSE");

  xy_socket_ops socket_ops = {.type = XY_OPS_CONNECT,
                              .connect_ = {tcp_sk, ip, port}};
  xy_syn_event_enqueue(&socket_ops);

  return established_retrieve(tcp_sk);
}

tcp_sock_t xy_accept(tcp_sock_t tcp_sk, uint32_t *ip, uint16_t *port) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, NULL,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, NULL,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_LISTEN, EINVAL, NULL,
                     "cannot accept when state is not TCP_LISTEN");
  xy_socket_ops socket_ops = {.type = XY_OPS_ACCEPT,
                              .connect_ = {tcp_sk, ip, port}};
  xy_syn_event_enqueue(&socket_ops);
  return established_take_next();
}

ssize_t xy_recv(tcp_sock_t tcp_sk, char *buf, size_t len, int flags) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, 0,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, 0,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_ESTABLISHED, EINVAL, 0,
                     "cannot receive when state is not TCP_ESTABLISHED");
  xy_return_errno_if(tcp_sk->tcb == NULL, EINVAL, 0,
                     "cannot receive when tcb is NULL");

  xy_socket_ops socket_ops = {.type = XY_OPS_RECV,
                              .recv_ = {tcp_sk, buf, len}};
  xy_syn_event_enqueue(&socket_ops);
  return socket_ops.recv_.ret;
}

ssize_t xy_send(tcp_sock_t tcp_sk, const char *buf, size_t len, int flags) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, 0,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, 0,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_ESTABLISHED, EINVAL, 0,
                     "cannot send when state is not TCP_ESTABLISHED");
  xy_return_errno_if(tcp_sk->tcb == NULL, EINVAL, 0,
                     "cannot send when tcb is NULL");

  xy_socket_ops socket_ops = {.type = XY_OPS_RECV,
                              .send_ = {tcp_sk, buf, len}};
  xy_syn_event_enqueue(&socket_ops);
  return socket_ops.send_.ret;
}

int xy_close(tcp_sock_t tcp_sk) {
  xy_return_errno_if(tcp_sk == NULL, EBADF, 0,
                     "the tcp_socket pointer is NULL");
  xy_return_errno_if(tcp_sk->id < 0 || tcp_sk->id > XY_MAX_TCP, EBADF, 0,
                     "the tcp socket id is invalid");
  xy_return_errno_if(tcp_sk->state != TCP_ESTABLISHED, EINVAL, 0,
                     "cannot connect when state is not TCP_ESTABLISHED");

  xy_socket_ops socket_ops = {.type = XY_OPS_CLOSE, .connect_ = {tcp_sk}};
  xy_syn_event_enqueue(&socket_ops);
  return 0;
}
