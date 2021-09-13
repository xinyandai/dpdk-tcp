#include "xy_api.h"

static xy_socket_map __socket_maps[XY_MAX_TCP];
static xy_list_node __free_sock_maps;

int xy_socket_map_init() {
  xy_list_init(&__free_sock_maps);

  for (int i = 0; i < XY_MAX_TCP; ++i) {
    xy_socket_map* sock_map = &__socket_maps[i];
    *sock_map = {.id = i,
                 .sock_type = XY_TCP_SOCK_UNUSED,
                 .event_poll = NULL,
                 .tcp_socket = NULL,
    };
    xy_list_add_tail(&__free_sock_maps, &sock_map->list_node);
  }
  return 0;
}

xy_tcp_socket* xy_allocate_tcp_socket() {
  if (xy_list_is_empty(&__free_sock_maps)) {
    return NULL;
  }
  auto sock_map = (xy_socket_map*)xy_list_take_head(&__free_sock_maps);

  sock_map->sock_type = XY_TCP_SOCK_TCP;
  sock_map->tcp_socket = xy_allocate(xy_tcp_socket);

  sock_map->tcp_socket->id = sock_map->id;
  return sock_map->tcp_socket;
}

xy_event_poll* xy_allocate_event_poll() {
  if (xy_list_is_empty(&__free_sock_maps)) {
    return NULL;
  }
  auto sock_map = (xy_socket_map*)xy_list_take_head(&__free_sock_maps);

  sock_map->sock_type = XY_TCP_SOCK_EPOLL;
  sock_map->event_poll = xy_allocate(xy_event_poll);

  sock_map->event_poll->id = sock_map->id;
  return sock_map->event_poll;
}

void xy_deallocate_tcp_socket(int sock_id) {
  xy_socket_map* sock_map = &__socket_maps[sock_id];

  xy_free(sock_map->tcp_socket);

  *sock_map = {.id = sock_id,
               .sock_type = XY_TCP_SOCK_UNUSED,
               .event_poll = NULL,
               .tcp_socket = NULL};

  xy_list_add_tail(&__free_sock_maps, &sock_map->list_node);
}

void deallocate_event_poll(int sock_id) {
  xy_socket_map* sock_map = &__socket_maps[sock_id];

  xy_free(sock_map->event_poll);

  *sock_map = {.id = sock_id,
               .sock_type = XY_TCP_SOCK_UNUSED,
               .event_poll = NULL,
               .tcp_socket = NULL};

  xy_list_add_tail(&__free_sock_maps, &sock_map->list_node);
}

xy_tcp_socket* xy_get_tcp_socket_by_id(int sock_id) {
  return __socket_maps[sock_id].tcp_socket;
}

xy_event_poll* xy_get_event_poll_by_id(int sock_id) {
  return __socket_maps[sock_id].event_poll;
}
