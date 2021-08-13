#ifndef __XY_API_H__
#define __XY_API_H__


#include <sys/types.h>
#include <sys/socket.h>

#include <stdint.h>
#include <signal.h>
#include <arpa/inet.h>
#include <inttypes.h>
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_cycles.h>
#include <rte_lcore.h>
#include <rte_byteorder.h>
#include <rte_mbuf.h>
#include <rte_ether.h>
#include <rte_arp.h>
#include <rte_icmp.h>
#include <rte_ip.h>
#include <rte_ip_frag.h>
#include <rte_tcp.h>

extern uint32_t __xy_this_ip;

#define BURST_SIZE 	1024

#define xy_return_if(condition, ret) { \
 if ((condition)) { \
	 return (ret); \
 } \
}

#define xy_rte_exit_if(condition, ...) { \
 if ((condition)) { \
	 rte_exit(EXIT_FAILURE, __VA_ARGS__); \
 } \
}

#define xy_warn_if(condition, ...) { \
 if ((condition)) { \
	 printf(__VA_ARGS__); \
 } \
}

#define xy_message_if(condition, ...) { \
 if ((condition)) { \
	 printf(__VA_ARGS__); \
 } \
}

inline int process_arp(struct rte_mbuf *mbuf, struct ether_hdr *eh, int len);
inline int process_icmp(struct rte_mbuf *mbuf,  struct ether_hdr *eh, struct ipv4_hdr *iph, int ipv4_hdrlen, int len);
inline int process_tcp(struct rte_mbuf *mbuf,  struct ether_hdr *eh, struct ipv4_hdr *iph, int ipv4_hdrlen, int len);;

int xy_socket(int domain, int type, int protocol);
int xy_bind(int sockid, const struct sockaddr *addr, socklen_t addrlen);
int xy_listen(int sockid, int backlog);
int xy_accept(int sockid, struct sockaddr *addr, socklen_t *addrlen);
ssize_t xy_recv(int sockid, char *buf, size_t len, int flags);
ssize_t xy_send(int sockid, const char *buf, size_t len);
int xy_close(int sockid);

void xy_tcp_setup(int argc, const char** argv);


int socket(int domain, int type, int protocol);
int bind(int sockid, const struct sockaddr *addr, socklen_t addrlen);
int listen(int sockid, int backlog);
int accept(int sockid, struct sockaddr *addr, socklen_t *addrlen);
ssize_t recv(int sockid, void *buf, size_t len, int flags);
ssize_t send(int sockid, const void *buf, size_t len, int flags);



#endif