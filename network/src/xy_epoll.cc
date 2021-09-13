#include <signal.h>
#include <time.h>
#include <unistd.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include "xy_api.h"
#include "xy_epoll.h"
#include "xy_syn_api.h"

int xy_epoll_create(int size) {
  xy_socket_ops socket_ops = {.type = XY_TCP_OPS_BIND,
                              .epoll_create_ = {size, 0}};
  xy_syn_event_enqueue(&socket_ops);
  xy_return_errno_if(socket_ops.err_no >= 0, socket_ops.err_no, -1, "err");
  return 0;
}

int xy_epoll_ctl(int epoll_id, int op, int sock,
                 xy_epoll_event *event) {
  xy_return_errno_if(epoll_id < 0 || epoll_id > XY_MAX_TCP, EBADF, -1,
                     "the epoll epoll_id is invalid");
  xy_event_poll *ep = xy_get_event_poll_by_id(epoll_id);

  xy_return_errno_if(ep == NULL, EBADF, -1, "NULL ep");
  xy_return_errno_if(sock == NULL, EBADF, -1, "NULL sock");
  xy_return_errno_if(!event && op != XY_EPOLL_CTL_DEL, EINVAL, -1,
                     "NULL event (not DEL ops)");

  if (op == XY_EPOLL_CTL_ADD) {
    pthread_mutex_lock(&ep->mtx);

    xy_epoll_item tmp;
    tmp.sock_id = sock;
    xy_epoll_item *epi = RB_FIND(_epoll_rb_socket, &ep->rb_tree, &tmp);
    if (epi) {
      pthread_mutex_unlock(&ep->mtx);
      return -1;
    }

    epi = xy_allocate(xy_epoll_item);
    if (!epi) {
      pthread_mutex_unlock(&ep->mtx);
      errno = -ENOMEM;
      return -1;
    }

    epi->sock_id = sock;
    memcpy(&epi->event, event, sizeof(xy_epoll_event));

    epi = RB_INSERT(_epoll_rb_socket, &ep->rb_tree, epi);
    assert(epi == NULL);
    ep->rb_num++;

    pthread_mutex_unlock(&ep->mtx);

  } else if (op == XY_EPOLL_CTL_DEL) {
    pthread_mutex_lock(&ep->mtx);

    xy_epoll_item tmp;
    tmp.sock_id = sock;
    xy_epoll_item *epi = RB_FIND(_epoll_rb_socket, &ep->rb_tree, &tmp);
    if (!epi) {
      pthread_mutex_unlock(&ep->mtx);
      return -1;
    }

    epi = RB_REMOVE(_epoll_rb_socket, &ep->rb_tree, epi);
    if (!epi) {
      pthread_mutex_unlock(&ep->mtx);
      return -1;
    }

    ep->rb_num--;
    free(epi);

    pthread_mutex_unlock(&ep->mtx);

  } else if (op == XY_EPOLL_CTL_MOD) {
    xy_epoll_item tmp;
    tmp.sock_id = sock;
    xy_epoll_item *epi = RB_FIND(_epoll_rb_socket, &ep->rb_tree, &tmp);
    if (epi) {
      epi->event.events = event->events;
      epi->event.events |= XY_EPOLLERR | XY_EPOLLHUP;
    } else {
      errno = -ENOENT;
      return -1;
    }

  } else {
    assert(0);
  }

  return 0;
}

int xy_epoll_wait(int epoll_id, xy_epoll_event *events, int max_events,
                  int timeout) {
  xy_return_errno_if(epoll_id < 0 || epoll_id > XY_MAX_TCP, EBADF, -1,
                     "the epoll epoll_id is invalid");
  xy_event_poll *ep = xy_get_event_poll_by_id(epoll_id);
  if (!ep || !events || max_events <= 0) {
    errno = -EINVAL;
    return -1;
  }

  if (pthread_mutex_lock(&ep->cdmtx)) {
    assert(0);
  }

  while (ep->ready_num == 0 && timeout != 0) {
    ep->waiting = 1;
    if (timeout > 0) {
      struct timespec deadline;

      clock_gettime(CLOCK_REALTIME, &deadline);
      if (timeout >= 1000) {
        int sec;
        sec = timeout / 1000;
        deadline.tv_sec += sec;
        timeout -= sec * 1000;
      }

      deadline.tv_nsec += timeout * 1000000;

      if (deadline.tv_nsec >= 1000000000) {
        deadline.tv_sec++;
        deadline.tv_nsec -= 1000000000;
      }

      int ret = pthread_cond_timedwait(&ep->cond, &ep->cdmtx, &deadline);
      if (ret && ret != ETIMEDOUT) {
        pthread_mutex_unlock(&ep->cdmtx);

        return -1;
      }
      timeout = 0;
    } else if (timeout < 0) {
      int ret = pthread_cond_wait(&ep->cond, &ep->cdmtx);
      if (ret) {
        pthread_mutex_unlock(&ep->cdmtx);

        return -1;
      }
    }
    ep->waiting = 0;
  }

  pthread_mutex_unlock(&ep->cdmtx);

  pthread_spin_lock(&ep->lock);

  int cnt = 0;
  int num = (ep->ready_num > max_events ? max_events : ep->ready_num);
  int i = 0;

  while (num != 0 && !xy_list_is_empty(&ep->ready_list)) {  /// EPOLLET

    auto epi = (xy_epoll_item *)xy_list_take_head(&ep->ready_list);
    epi->ready = 0;

    memcpy(&events[i++], &epi->event, sizeof(xy_epoll_event));

    num--;
    cnt++;
    ep->ready_num--;
  }

  pthread_spin_unlock(&ep->lock);

  return cnt;
}


int epoll_event_callback(int sock, uint32_t event) {
  xy_event_poll * ep = xy_get_event_poll_by_id(sock);

  xy_epoll_item tmp;
  tmp.sock_id = sock;
  xy_epoll_item *epi = RB_FIND(_epoll_rb_socket, &ep->rb_tree, &tmp);
  if (!epi) {
    assert(0);
  }
  if (epi->ready) {
    epi->event.events |= event;
    return 1;
  }


  pthread_spin_lock(&ep->lock);
  epi->ready = 1;
  xy_list_add_head(&ep->ready_list, (xy_list_node*)epi);
  ep->ready_num ++;
  pthread_spin_unlock(&ep->lock);

  pthread_mutex_lock(&ep->cdmtx);

  pthread_cond_signal(&ep->cond);
  pthread_mutex_unlock(&ep->cdmtx);
  return 0;
}

static int epoll_destroy(xy_event_poll* ep) {
  while (!xy_list_is_empty(&ep->ready_list)) {
    xy_list_take_head(&ep->ready_list);
  }

  //remove rb-tree
  pthread_mutex_lock(&ep->mtx);

  for (;;) {
    xy_epoll_item *epi = RB_MIN(_epoll_rb_socket, &ep->rb_tree);
    if (epi == NULL) break;

    epi = RB_REMOVE(_epoll_rb_socket, &ep->rb_tree, epi);
    free(epi);
  }
  pthread_mutex_unlock(&ep->mtx);

  return 0;
}

int xy_epoll_close_socket(int sock) {
  xy_event_poll * ep = xy_get_event_poll_by_id(sock);
  if (!ep) {
    errno = -EINVAL;
    return -1;
  }

  epoll_destroy(ep);

  pthread_cond_destroy(&ep->cond);
  pthread_mutex_destroy(&ep->mtx);
  pthread_spin_destroy(&ep->lock);

  free(ep);

  return 0;
}
