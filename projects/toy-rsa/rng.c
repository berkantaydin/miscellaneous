#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "aes.h"

static struct aes_ctx rng_key;
static unsigned long ctr[ULONGS_PER_AES_BLOCK] = {0};

void init_rng(void)
{
	int r,fd = open(ENTROPY_SOURCE,O_RDONLY);
	unsigned char *key[16] = {0};

	if (fd == -1)
		fatal("Couldn't open entropy source\n");
	r = read(fd, &key, 16);
	if (r != 16)
		fatal("Weirdness reading from entropysource\n");
	close(fd);

	aes128_expand_key(&rng_key, &key);
}

static void put_next_counter(void *mem)
{
	memcpy(mem, ctr, 16);
	for (int i = 0; i < ULONGS_PER_AES_BLOCK; i++)
		if (++ctr[i])
			goto ctr_not_over;

	/* Counter wrapped - get new key
	 * This can never happen in practice? */
	init_rng();

	ctr_not_over:
	return;
}

void rng_fill_mem(void *mem, int len)
{
	char *cmem = mem;
	if (len & 0x0f)
		fatal("rng_fill_mem() len must be multiple of 16\n");

	for (int i = 0; i < len; i += 16) {
		put_next_counter(cmem + i);
		aes128_encrypt(&rng_key, cmem + i);
	}
}
