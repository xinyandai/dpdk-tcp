//
// Created by xydai on 2021/8/23.
//
#pragma once
#ifndef DPDK_TCP_INCLUDE_XY_TCP_PORT_H_
#define DPDK_TCP_INCLUDE_XY_TCP_PORT_H_
#include <stdint.h>
#include <mutex>
typedef uint8_t port_flag_t;
#define PORTS_OCCUPIED_SPACE (XY_MAX_TCP / (8 * sizeof(port_flag_t)))
extern port_flag_t xy_ports_occupied[PORTS_OCCUPIED_SPACE];
extern std::mutex xy_port_mutex;

static inline uint16_t port_flag_index(uint16_t port) {
  return port / (8 * sizeof(port_flag_t));
}

static inline port_flag_t port_index_mask(uint16_t port) {
  return 1 << port_flag_t(port % sizeof(port_flag_t));
}

/**
 *
 * \param port the port to be registered
 * \return 0 on success, -1 if the port is registered
 */
inline int port_register(uint16_t port) {
  uint16_t index = port_flag_index(port);
  port_flag_t mask = port_index_mask(port);

  std::lock_guard<std::mutex> lock(xy_port_mutex);
  if xy_unlikely(xy_ports_occupied[index] & mask) {
    return -1;
  } else {
    xy_ports_occupied[index] |= mask;
    return 0;
  }
}
#endif  // DPDK_TCP_INCLUDE_XY_TCP_PORT_H_
