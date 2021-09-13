#include "xy_api.h"
#include "xy_syn_api.h"

int xy_socket(int domain, int type, int protocol) {
  xy_socket_ops socket_ops = {.type = XY_TCP_OPS_CREATE, .create_ = {}};
  xy_syn_event_enqueue(&socket_ops);
  xy_return_errno_if(socket_ops.err_no >= 0, socket_ops.err_no, -1, "err");
  // init tcp_sk
  return socket_ops.create_.sock_id;
}

int xy_bind(int sock_id, uint32_t ip, uint16_t port) {
  xy_socket_ops socket_ops = {.type = XY_TCP_OPS_BIND,
                              .bind_ = {sock_id, ip, port}};
  xy_syn_event_enqueue(&socket_ops);
  xy_return_errno_if(socket_ops.err_no >= 0, socket_ops.err_no, -1, "err");
  return 0;
}

int xy_listen(int sock_id, int backlog) {
  xy_socket_ops socket_ops = {.type = XY_TCP_OPS_LISTEN, .bind_ = {sock_id}};
  xy_syn_event_enqueue(&socket_ops);
  xy_return_errno_if(socket_ops.err_no >= 0, socket_ops.err_no, -1, "err");
  return 0;
}

int xy_connect(int sock_id, uint32_t *ip, uint16_t *port) {
  xy_socket_ops socket_ops = {.type = XY_TCP_OPS_CONNECT,
                              .connect_ = {sock_id, ip, port}};
  xy_syn_event_enqueue(&socket_ops);
  xy_return_errno_if(socket_ops.err_no >= 0, socket_ops.err_no, -1, "err");
  xy_tcp_socket *tcp_sk = xy_get_tcp_socket_by_id(sock_id);
  return established_retrieve(tcp_sk)->id;
}

int xy_accept(int sock_id, uint32_t *ip, uint16_t *port) {
  xy_socket_ops socket_ops = {.type = XY_TCP_OPS_ACCEPT,
                              .connect_ = {sock_id, ip, port}};
  xy_syn_event_enqueue(&socket_ops);
  xy_return_errno_if(socket_ops.err_no >= 0, socket_ops.err_no, -1, "err");
  return established_take_next()->id;
}

ssize_t xy_recv(int sock_id, char *buf, size_t len, int flags) {
  xy_socket_ops socket_ops = {.type = XY_TCP_OPS_RECV,
                              .recv_ = {sock_id, buf, len}};
  xy_syn_event_enqueue(&socket_ops);
  xy_return_errno_if(socket_ops.err_no >= 0, socket_ops.err_no, 0, "err");
  return socket_ops.recv_.ret;
}

ssize_t xy_send(int sock_id, const char *buf, size_t len, int flags) {
  xy_socket_ops socket_ops = {.type = XY_TCP_OPS_RECV,
                              .send_ = {sock_id, buf, len}};
  xy_syn_event_enqueue(&socket_ops);
  xy_return_errno_if(socket_ops.err_no >= 0, socket_ops.err_no, 0, "err");
  return socket_ops.send_.ret;
}

int xy_close(int sock_id) {
  xy_socket_ops socket_ops = {.type = XY_TCP_OPS_CLOSE, .connect_ = {sock_id}};
  xy_syn_event_enqueue(&socket_ops);
  xy_return_errno_if(socket_ops.err_no >= 0, socket_ops.err_no, -1, "err");
  return 0;
}
