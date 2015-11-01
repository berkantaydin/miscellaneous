#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <unistd.h>
#include <math.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <assert.h>
#include <poll.h>

uint64_t htonll(uint64_t);
uint64_t ntohll(uint64_t);
int init_udp_socket(int, char*, int);
int init_in_tcp_socket(int);

int datasock, controlsock, filefd, interface_mss, data_mss;
uint64_t total_bytes_missed;

struct data_missed {
	uint64_t offset;
	uint64_t len;
};

void log_data_missed(uint64_t offset, uint64_t len, struct data_missed *data_missed)
{
	data_missed->offset = htonll(offset);
	data_missed->len = htonll(len);
	total_bytes_missed += len;
	return;
}

int reconcile_data_loss(struct data_missed *data_missed, uint64_t blocks_missed)
{
	uint64_t bytes_to_write, block_len, block_offset, block_cur, missed_nbo;
	uint64_t blocks_left = blocks_missed;
	int real_write, real_read;
	char *buffer, *data;

	missed_nbo = htonll(blocks_missed);
	real_write = write(controlsock, &missed_nbo, 8);

	if(!blocks_missed) {
		printf("Got all the data\n");
		return 0;
	}

	printf("Missed %" PRIu64 " blocks of data - %" PRIu64 " bytes total\n", blocks_missed, total_bytes_missed);

	buffer = (char *) data_missed;
	bytes_to_write = blocks_missed * sizeof(struct data_missed);

	while(bytes_to_write) {
		real_write = write(controlsock, buffer, bytes_to_write);
		bytes_to_write -= real_write;
		buffer += real_write;
	}

	block_cur = 0;
	data = calloc(1, 32768);

	while(block_cur < blocks_left) {
		block_offset = ntohll(data_missed[block_cur].offset);
		lseek(filefd, block_offset, SEEK_SET);
		block_len = ntohll(data_missed[block_cur].len);
		printf("Receivng missed block #%" PRIu64 " (offset 0x%016" PRIu64 ", length %" PRIu64 ")\n", block_cur + 1, block_offset, block_len);
		while(block_len) {
			real_read = read(controlsock, data, (block_len > 32768) ? 32768 : block_len);
			block_len -= real_read;
			real_write = write(filefd, data, real_read);
			printf("Read %i from server, %" PRIu64 " left...\n", real_read, block_len);
		}
		block_cur++;
	}
	return -1;
}

void recv_main(int nargs, char* args[])
{
	uint64_t filelen, left_to_recieve, offset_expected, offset_cur, blocks_missed;
	uint64_t *offset;
	int real_read, real_write;
	char *buffer, *buffer_data;
	char *data_missed_block;
	struct data_missed *data_missed_arr;
	struct pollfd pollstruct;

	controlsock = init_in_tcp_socket(atoi(args[3]));
	datasock = init_udp_socket(atoi(args[3]), args[4], atoi(args[5]));
	filefd = open(args[2],O_RDWR|O_CREAT|O_TRUNC,S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);

	pollstruct.fd = datasock;
	pollstruct.events = POLLIN;

	interface_mss = 1400;
	data_mss = interface_mss - 8;

	real_read = read(controlsock, &filelen, 8);
	filelen = ntohll(filelen);
	left_to_recieve = filelen;

	buffer = calloc(1, interface_mss);
	buffer_data = buffer + 8;
	offset = (uint64_t *) buffer;

	offset_expected = 0;

	printf("Sender claims filesize to be %" PRIu64 " bytes\n", filelen);

	data_missed_block = calloc((filelen / data_mss / 2) + 1, sizeof(struct data_missed));
	data_missed_arr = (struct data_missed *) data_missed_block;
	blocks_missed = 0;

	while(left_to_recieve) {
		if(!poll(&pollstruct, 1, 2000)) {
			log_data_missed(offset_expected, filelen - offset_expected, &data_missed_arr[blocks_missed++]);
			printf("Expected offset 0x%016" PRIu64 ", but got EOF (missed %" PRIu64 " bytes - %" PRIu64 " packets)\n",
				offset_expected, (filelen - offset_expected), (filelen - offset_expected) / data_mss);
			break;
		}
		real_read = read(datasock, buffer, interface_mss);
		offset_cur = ntohll(*offset);
		left_to_recieve -= (real_read - 8);
		if(offset_cur != offset_expected) {
			printf("Expected offset 0x%016" PRIu64 ", but got 0x%016" PRIu64 " (missed %" PRIu64 " bytes - %" PRIu64 " packets)\n",
				offset_expected, offset_cur, (offset_cur - offset_expected), (offset_cur - offset_expected) / data_mss);
			lseek(filefd, offset_cur, SEEK_SET);
			left_to_recieve -= (offset_cur - offset_expected);
			log_data_missed(offset_expected, (offset_cur - offset_expected), &data_missed_arr[blocks_missed++]);
		}
		offset_expected = offset_cur + data_mss;
		real_write = write(filefd, buffer_data, real_read - 8);
		assert((real_read - 8) == real_write);
	}
	reconcile_data_loss(data_missed_arr, blocks_missed);

	return;
}
