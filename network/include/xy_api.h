#pragma once
#ifndef __DPDK_TCP_INCLUDE_XY_API_H__
#define __DPDK_TCP_INCLUDE_XY_API_H__

#include <sys/socket.h>
#include <sys/types.h>

#include <arpa/inet.h>
#include <inttypes.h>
#include <rte_arp.h>
#include <rte_byteorder.h>
#include <rte_cycles.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_ether.h>
#include <rte_icmp.h>
#include <rte_ip.h>
#include <rte_ip_frag.h>
#include <rte_lcore.h>
#include <rte_mbuf.h>
#include <rte_tcp.h>
#include <signal.h>
#include <stdint.h>

#include "xy_init.h"
#include "xy_arp.h"
#include "xy_eth.h"
#include "xy_icmp.h"
#include "xy_ip.h"
#include "xy_tcp.h"

#include "xy_list.h"

#include "xy_macros.h"
#include "xy_manage.h"
#include "xy_socks.h"
#include "xy_struct.h"
#include "xy_tcp_port.h"

#include "xy_tcp_wnd_hdl.h"


int xy_socket(int domain, int type, int protocol);
int xy_bind(int tcp_sk, uint32_t ip, uint16_t port);
int xy_listen(int tcp_sk, int backlog);
int xy_accept(int tcp_sk, uint32_t *ip, uint16_t *port);
int xy_connect(int tcp_sk, uint32_t *ip, uint16_t *port);
ssize_t xy_recv(int tcp_sk, char *buf, size_t len, int flags);
ssize_t xy_send(int tcp_sk, const char *buf, size_t len);
int xy_close(int tcp_sk);

#endif