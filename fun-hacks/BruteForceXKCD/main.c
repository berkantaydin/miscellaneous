/* skein1024.c */
/*
    This file is part of the AVR-Crypto-Lib.
    Copyright (C) 2009  Daniel Otte (daniel.otte@rub.de)

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
/*
 * \author  Daniel Otte
 * \email   daniel.otte@rub.de
 * \date    2009-03-12
 * \license GPLv3 or later
 * 
 */

/* XKCD hash matching and uploading kludged together
 * by Calvin Owens <jcalvinowens@gmail.com> */

#define _GNU_SOURCE
#include <time.h>
#include <sys/mman.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include "ubi.h"
#include "skein.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>

#define HASH_STR_LEN 32

const char target[128] = {'\x5b','\x4d','\xa9','\x5f','\x5f','\xa0','\x82','\x80','\xfc','\x98','\x79','\xdf','\x44','\xf4','\x18','\xc8','\xf9','\xf1','\x2b','\xa4','\x24','\xb7','\x75','\x7d','\xe0','\x2b','\xbd','\xfb','\xae','\x0d','\x4c','\x4f','\xdf','\x93','\x17','\xc8','\x0c','\xc5','\xfe','\x04','\xc6','\x42','\x90','\x73','\x46','\x6c','\xf2','\x97','\x06','\xb8','\xc2','\x59','\x99','\xdd','\xd2','\xf6','\x54','\x0d','\x44','\x75','\xcc','\x97','\x7b','\x87','\xf4','\x75','\x7b','\xe0','\x23','\xf1','\x9b','\x8f','\x40','\x35','\xd7','\x72','\x28','\x86','\xb7','\x88','\x69','\x82','\x6d','\xe9','\x16','\xa7','\x9c','\xf9','\xc9','\x4c','\xc7','\x9c','\xd4','\x34','\x7d','\x24','\xb5','\x67','\xaa','\x3e','\x23','\x90','\xa5','\x73','\xa3','\x73','\xa4','\x8a','\x5e','\x67','\x66','\x40','\xc7','\x9c','\xc7','\x01','\x97','\xe1','\xc5','\xe7','\xf9','\x02','\xfb','\x53','\xca','\x18','\x58','\xb6'};

void skein1024_init(skein1024_ctx_t* ctx, uint16_t outsize_b)
{
	skein_config_t conf;
	uint8_t null[UBI1024_BLOCKSIZE_B];
	memset(null, 0, UBI1024_BLOCKSIZE_B);
	memset(&conf, 0, sizeof(skein_config_t));
	conf.schema[0] = 'S';
	conf.schema[1] = 'H';
	conf.schema[2] = 'A';
	conf.schema[3] = '3';
	conf.version = 1;
	conf.out_length = outsize_b;
	ctx->outsize_b = outsize_b;
	ubi1024_init(&(ctx->ubictx), null, UBI_TYPE_CFG);
	ubi1024_lastBlock(&(ctx->ubictx), &conf, 256);
	ubi1024_init(&(ctx->ubictx), ctx->ubictx.g, UBI_TYPE_MSG);
}

void skein1024_nextBlock(skein1024_ctx_t* ctx, const void* block)
{
	ubi1024_nextBlock(&(ctx->ubictx), block);
}

void skein1024_lastBlock(skein1024_ctx_t* ctx, const void* block, uint16_t length_b)
{
	ubi1024_lastBlock(&(ctx->ubictx), block, length_b);
}

void skein1024_ctx2hash(void* dest, skein1024_ctx_t* ctx)
{
	ubi1024_ctx_t uctx;
	uint16_t outsize_b;

	uint64_t counter=0;
	uint8_t outbuffer[UBI1024_BLOCKSIZE_B];
	ubi1024_init(&(ctx->ubictx), ctx->ubictx.g, UBI_TYPE_OUT);

	outsize_b = ctx->outsize_b;
	while(1){
		memcpy(&uctx, &(ctx->ubictx), sizeof(ubi1024_ctx_t));
		ubi1024_lastBlock(&uctx, &counter, 64);
		ubi1024_ctx2hash(outbuffer, &uctx);
		if(outsize_b<=UBI1024_BLOCKSIZE){
			memcpy(dest, outbuffer, (ctx->outsize_b+7)/8);
			break;
		}else{
			memcpy(dest, outbuffer, UBI1024_BLOCKSIZE_B);
			dest = (uint8_t*)dest + UBI1024_BLOCKSIZE_B;
			outsize_b -= UBI1024_BLOCKSIZE;
			counter++;
		}
	}
}

void skein1024(void* dest, uint16_t outlength_b, const void* msg, uint32_t length_b)
{
	skein1024_ctx_t ctx;
	skein1024_init(&ctx, outlength_b);
	while(length_b>SKEIN1024_BLOCKSIZE){
		skein1024_nextBlock(&ctx, msg);
		msg = (uint8_t*)msg + SKEIN1024_BLOCKSIZE_B;
		length_b -= SKEIN1024_BLOCKSIZE;
	}
	skein1024_lastBlock(&ctx, msg, length_b);
	skein1024_ctx2hash(dest, &ctx);
}

unsigned short do_hash(char *phrase, char *out)
{
	unsigned short non_matching_bits = 0;

	skein1024(out, 1024, phrase, strlen(phrase) << 3);

	/* FIXME: Super pedantic - there are better ways to do this... */
	for(int i = 0; i < 128; i++) {
		non_matching_bits += (unsigned int) (out[i] & 0x01) ^ (target[i] & 0x01);
		non_matching_bits += (unsigned int) ((out[i] & 0x02) ^ (target[i] & 0x02)) >> 1;
		non_matching_bits += (unsigned int) ((out[i] & 0x04) ^ (target[i] & 0x04)) >> 2;
		non_matching_bits += (unsigned int) ((out[i] & 0x08) ^ (target[i] & 0x08)) >> 3;
		non_matching_bits += (unsigned int) ((out[i] & 0x10) ^ (target[i] & 0x10)) >> 4;
		non_matching_bits += (unsigned int) ((out[i] & 0x20) ^ (target[i] & 0x20)) >> 5;
		non_matching_bits += (unsigned int) ((out[i] & 0x40) ^ (target[i] & 0x40)) >> 6;
		non_matching_bits += (unsigned int) ((out[i] & 0x80) ^ (target[i] & 0x80)) >> 7;
	}

	return non_matching_bits;
}

void make_byte_printable(char *byte)
{
	char hi, lo, a, b, c, cond1, cond2;

	/* Kill the high unloved high bit */
	byte[0] &= 0x7f;

	/* If bits 6 and 7 are lo, set them randomly to 01, 10, or 11
	 * based on the high bits of the next 3 bytes */
	hi = (byte[0] & 0x40);
	lo = (byte[0] & 0x20);
	cond1 = ~((hi >> 1) | (lo << 1) | byte[0]);
	a = ((byte[1] & 0x80) >> 2);
	b = (((byte[2] & 0x80) >> 1) | ((byte[3] & 0x80) >> 1));
	c = ((b ^ 0x40) >> 1);
	byte[0] |= cond1 & (a | b | c);

	/* If we got 0x7f, randomly distribute across 01xxxxx by setting bit 6 and
	 * XORing bits 1-5 with the next byte */
	cond2 = !(byte[0] ^ 0x7f);
	byte[0] &= ~((cond2) << 6 | ((cond2 << 4 | cond2 << 3 | cond2 << 2 | cond2 << 1 | cond2) & (byte[1] & 0x1f)));
}

void init_string(char *str)
{
	int rd, rfd = open("/dev/urandom", O_RDONLY);
	rd = read(rfd, str, HASH_STR_LEN + 3);
	close(rfd);

	for (int i = 0; i < HASH_STR_LEN; i++)
		make_byte_printable(&str[i]);

	str[HASH_STR_LEN] = '\0';
}

void inc_string(char *str)
{
	for (int i = 0; i < HASH_STR_LEN - 1; i++) {
		if (str[i]++ > 126) {
			str[i] = 0x20;
			continue;
		}
		break;
	}
}

void post_to_wobsite(char *str)
{
	#if 0 /* Since the contest is over now */
	struct sockaddr_in to_address;
	char *post_msg = NULL;
	int ret, post_len, fd;

	fd = socket(AF_INET, SOCK_STREAM, 0);
	to_address.sin_family = AF_INET;
	to_address.sin_port = htons(80);
	to_address.sin_addr.s_addr = inet_addr("107.6.89.242"); /* Static linking breaks getaddrbyname() on old RHEL machines */
	if(connect(fd, (struct sockaddr *)&to_address, sizeof(struct sockaddr_in)) < 0) {
		printf(" UNABLE TO CONNECT TO XKCD!! OMGZ!\n");
		return;
	}

	post_len = asprintf(&post_msg, "POST /?edu=smu.edu HTTP/1.1\r\nHost: almamater.xkcd.com\r\nUser-Agent: Your Mother, v1.0\r\nAccept: */*\r\nContent-Length: %i\r\nContent-Type: application/x-www-form-urlencoded\r\n\r\nhashable=%s", HASH_STR_LEN + 8, str);
	ret = write(fd, post_msg, post_len); /* Meh. */

	free(post_msg);
	close(fd);
	#endif
}

int main(int args, char **argv)
{
	unsigned short got, target, *best_so_far;
	unsigned long *hashes_computed; /* Will overflow on x86-32 */
	time_t init, acct;
	char *str, *out;

	if (args < 2 || args > 3)
		return printf("Usage: %s #threads [target]\nIf [target] given, only hashes at or below that number will be printed\n", argv[0]);

	if (args == 3)
		target = atoi(argv[2]);
	else
		target = 512;

	setbuf(stdout, NULL);

	/* Putting a lock on these would introduce a stupid amount of overhead.
	 * Turns out GCC emits 'addq $0x1,(%rdx)' on x86 anyway, so no race. ;) */
	hashes_computed = mmap(NULL, sizeof(*hashes_computed), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);
	best_so_far = mmap(NULL, sizeof(*best_so_far), PROT_READ|PROT_WRITE, MAP_SHARED|MAP_ANON, -1, 0);

	*hashes_computed = 0;
	*best_so_far = 512;

	for(int i = 0; i < atoi(argv[1]) - 1; i++) {
		if (!fork())
			break;
	}

	init = time(NULL);
	sleep(1);

	str = calloc(1, HASH_STR_LEN + 1);
	init_string(str);

	out = calloc(1, 128);

	while(1) {
		inc_string(str);
		got = do_hash(str, out);
		*hashes_computed += 1;

		if (got <= target && got < *best_so_far) {
			*best_so_far = got;
			acct = time(NULL);
			printf("FOUND: \"%s\": %i [%li hashes computed - %li per second]\n", str, got, *hashes_computed, *hashes_computed / (long) (acct - init));
			post_to_wobsite(str);
		}
	}

	return 0;
}
