#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <math.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <assert.h>

/* Functions to convert byte-ordering for 64-bit integers */
inline uint64_t htonll(uint64_t arg)
{
	#ifdef __x86_64__
	asm("bswap %0" : "=r" (arg) : "0" (arg));
	return arg;
	#else
	return ((uint64_t) htonl(*(((uint32_t*) &arg) + 1))) | (uint64_t) htonl(arg) << 32;
	#endif
}

inline uint64_t ntohll(uint64_t arg)
{
	#ifdef __x86_64__
	asm("bswap %0" : "=r" (arg) : "0" (arg));
	return arg;
	#else
	return ((uint64_t) ntohl(*(((uint32_t*) &arg) + 1))) | (uint64_t) ntohl(arg) << 32;
	#endif
}

int init_socket(int type, short src_port)
{
	int datasock = socket(AF_INET, type, 0);
	struct sockaddr_in bind_address;

	if (datasock < 0)
		return -1;

	bind_address.sin_family = AF_INET;
	bind_address.sin_port = htons(src_port);
	bind_address.sin_addr.s_addr = INADDR_ANY;
	if(bind(datasock, (struct sockaddr*) &bind_address, sizeof(bind_address)) < 0)
		return -1;
	return datasock;
}

int init_udp_socket(short src_port, char *dst_address, short dst_port)
{
	int fd = init_socket(SOCK_DGRAM, src_port);
	struct sockaddr_in to_address;

	to_address.sin_family = AF_INET;
	to_address.sin_port = htons(dst_port);
	to_address.sin_addr.s_addr = inet_addr(dst_address);
	if(connect(fd, (struct sockaddr*) &to_address, sizeof(to_address)) < 0)
		return -1;

	return fd;
}

int init_out_tcp_socket(short src_port, char *dst_address, short dst_port)
{
	int fd = init_socket(SOCK_STREAM, src_port);
	struct sockaddr_in to_address;

	to_address.sin_family = AF_INET;
	to_address.sin_port = htons(dst_port);
	to_address.sin_addr.s_addr = inet_addr(dst_address);
	if(connect(fd, (struct sockaddr *)&to_address, sizeof(struct sockaddr_in)) < 0)
		return -1;

	return fd;
}

int init_in_tcp_socket(short src_port)
{
	int fd = init_socket(SOCK_STREAM, src_port);
	int fd2;

	if(listen(fd, 1))
		return -1;

	fd2 = accept(fd, NULL, NULL);

	close(fd);
	return fd2;
}
