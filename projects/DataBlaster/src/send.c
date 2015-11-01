#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <stddef.h>
#include <math.h>
#include <time.h>
#include <assert.h>
#include <unistd.h>
#include <inttypes.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/sendfile.h>

uint64_t htonll(uint64_t);
uint64_t ntohll(uint64_t);
int init_udp_socket(int, char*, int);
int init_out_tcp_socket(int, char*, int);

/* Global variables */
int datasock, controlsock, filefd, interface_mss, data_mss;
long bytes_per_second, packets_per_second, seconds, nanoseconds;
long double packet_interval;
struct timespec time_to_sleep;

struct data_missed {
	uint64_t offset;
	uint64_t len;
};

int resupply_missed_data(void)
{
	uint64_t missed_blocks, bytes_to_read, block_cur, block_offset, block_len;
	char *buffer, *tmp;
	int real_read, real_spliced;
	struct data_missed *data_missed;

	real_read = read(controlsock, &missed_blocks, 8);
	missed_blocks = ntohll(missed_blocks);

	if(!missed_blocks) {
		printf("Client reports no data loss! Yay!\n");
		return 0;
	}

	printf("Reconciling data - missed %" PRIu64 " blocks\n", missed_blocks);

	buffer = calloc(missed_blocks, sizeof(struct data_missed));
	bytes_to_read = missed_blocks * sizeof(struct data_missed);
	tmp = buffer;

	while(bytes_to_read) {
		real_read = read(controlsock, tmp, bytes_to_read);
		bytes_to_read -= real_read;
		tmp += real_read;
	}

	data_missed = (struct data_missed*) buffer;

	block_cur = 0;

	while(block_cur < missed_blocks) {
		block_offset = ntohll(data_missed[block_cur].offset);
		lseek(filefd, block_offset, SEEK_SET);
		block_len = ntohll(data_missed[block_cur].len);
		printf("Resending block #%" PRIu64 " (offset 0x%016" PRIu64 ", length %" PRIu64 ")\n", block_cur + 1, block_offset, block_len);
		while(block_len) {
			real_spliced = sendfile(controlsock, filefd, NULL, block_len);
			block_len -= real_spliced;
			printf("Spliced %i to client, %" PRIu64 " left...\n", real_spliced, block_len);
		}
		block_cur++;
	}

	return -1;
}

void send_main(int nargs, char* args[])
{
	struct stat *filestat;
	uint64_t filelen, left_to_send, offset_cur;
	uint64_t *offset;
	char *buffer;
	int real_read, real_write;

	bytes_per_second = atoi(args[6]) * (1024 * 1024);
	interface_mss = 1400;
	data_mss = interface_mss - 8;
	packets_per_second = bytes_per_second / interface_mss + 1;
	packet_interval = ((long double) 1) / packets_per_second;
	seconds = (truncl(packet_interval));
	nanoseconds = ((packet_interval - truncl(packet_interval)) * (long double) 1000000000);

	time_to_sleep.tv_sec = (time_t) seconds;
	time_to_sleep.tv_nsec = nanoseconds;

	datasock = init_udp_socket(atoi(args[3]), args[4], atoi(args[5]));
	controlsock = init_out_tcp_socket(atoi(args[3]), args[4], atoi(args[5]));
	filefd = open(args[2],O_RDONLY);

	filestat = calloc(1,sizeof(struct stat));
	fstat(filefd,filestat);

	filelen = (uint64_t) filestat->st_size;
	left_to_send = filelen;
	offset_cur = 0;

	buffer = calloc(1, interface_mss);
	offset = (uint64_t*) buffer;
	buffer += 8;

	printf("Sending %" PRIu64 " packets (%li %i byte packets/second: %li B/s)\nOne packet every %li nanoseconds\nFile Size: %" PRIu64 "\n",
	 filelen / interface_mss + 1, packets_per_second, interface_mss, packets_per_second * interface_mss, nanoseconds,
	 filelen);

	filelen = htonll(filelen);

	real_write = write(controlsock, &filelen, 8);

	sleep(1);

	while(left_to_send) {
		real_read = read(filefd, buffer, interface_mss - 8);
		*offset = htonll(offset_cur);
		left_to_send -= real_read;
		offset_cur += real_read;
		nanosleep(&time_to_sleep, NULL);
		real_write = write(datasock, offset, real_read + 8);
		assert(real_read == real_write - 8);
	}

	resupply_missed_data();

	return;
}
