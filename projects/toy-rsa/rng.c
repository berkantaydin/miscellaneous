/*
 * bfi.c: Simple RNG, random AES128 encryption of a monotonic counter.
 *
 * Copyright (C) 2016 Calvin Owens <jcalvinowens@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; version 2 only.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "include/common.h"
#include "include/aes.h"
#include "include/rng.h"

#define AES_BLOCKSIZE 16
#define ULONGS_PER_AES_BLOCK ((AES_BLOCKSIZE / sizeof(unsigned long)))

static struct aes_ctx *rng_key;
static unsigned long ctr[ULONGS_PER_AES_BLOCK];

void init_rng(void)
{
	int fd, ret, tmp_len;
	char entropy[AES_BLOCKSIZE * 2], *tmp_ptr;

	fd = open(ENTROPY_SOURCE, O_RDONLY);
	if (fd == -1)
		fatal("Couldn't open entropy source\n");


	printf("Waiting for %d bytes of entropy: ", AES_BLOCKSIZE * 2);

	tmp_ptr = entropy;
	tmp_len = AES_BLOCKSIZE * 2;
	do {
		ret = read(fd, tmp_ptr, tmp_len);
		if (ret == -1)
			fatal("Error reading from entropy source: %m\n");

		printf("%ld/%d ", tmp_ptr + ret - entropy, AES_BLOCKSIZE * 2);
		tmp_ptr += ret;
		tmp_len -= ret;
	} while (tmp_len);

	puts("RNG ready!");
	close(fd);

	rng_key = aes128_expand_key(entropy);
	memcpy(ctr, &entropy[AES_BLOCKSIZE], AES_BLOCKSIZE);
}

void free_rng(void)
{
	free(rng_key);
}

static void put_next_counter(void *mem)
{
	unsigned i;

	memcpy(mem, ctr, 16);
	for (i = 0; i < ULONGS_PER_AES_BLOCK; i++)
		if (++ctr[i])
			break;
}

void rng_fill_mem(void *mem, int len)
{
	char *cmem = mem;
	int i;

	if (len % AES_BLOCKSIZE)
		fatal("Length %d isn't multiple of %d\n", len, AES_BLOCKSIZE);

	for (i = 0; i < len; i += 16) {
		put_next_counter(cmem + i);
		aes128_encrypt(rng_key, cmem + i);
	}
}
