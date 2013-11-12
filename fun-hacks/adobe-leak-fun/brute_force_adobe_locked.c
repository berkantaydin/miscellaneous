/* brute_force_adobe.c:	Brute force the 3DES key used to encrypt passwords in
 * 			the recently leaked Adobe database.
 *
 * In practice, this is pretty useless. However, if enough people run this,
 * somebody will *eventually* find the key. Maybe. Actually, probably not.
 *
 * Copyright (C) 2013 Calvin Owens
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of version 2 of the GNU General Public License as published by the
 * Free Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THIS SOFTWARE. */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdint.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <openssl/des.h>

/* So we can be clever with integer instructions
 * DES_cblock is unsigned char[8] */
typedef union u {uint32_t num32; uint64_t num64; DES_cblock des;} DES_cblock_aligned;

/* Compile with -DTEST to make sure it's working */
#ifdef TEST
	static DES_cblock_aligned ciphertext = {.des = {'\x28', '\x3A', '\x52', '\x71', '\x2E', '\xCD', '\x40', '\x93'}};
	static char *plaintext = "12345678";
#else
	static DES_cblock_aligned ciphertext = {.des = {'\x11', '\x0E', '\xDF', '\x22', '\x94', '\xFB', '\x8B', '\xF4'}};
	static char *plaintext = "123456";
#endif

/* First 4 characters of plaintext as integer */
#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	static uint32_t plain_as_int = 0x34333231;
#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	static uint32_t plain_as_int = 0x31323334;
#endif

/* Because my university is still running 2.6.9 on its servers... */
#ifndef CLOCK_MONOTONIC_COARSE
#warning "Your kernel is ancient. You should fix that."
#define CLOCK_MONOTONIC_COARSE CLOCK_MONOTONIC
#endif
#if __GCC_HAVE_SYNC_COMPARE_AND_SWAP_4 != 1
#error "Your GCC is prehistoric. You can't build this."
#endif

/* Lame boring spinlock.
 * The assumption is that you're only running as many threads as you have
 * physical CPUs. This probably doesn't scale terribly well either.
 * Ought to use futexes, could be a win on hyperthreaded CPU's, idk. */
static void take_spinlock(int *lock)
{
	while (!__sync_bool_compare_and_swap(lock, 0, 1));
}

static void release_spinlock(int *lock)
{
	__sync_bool_compare_and_swap(lock, 1, 0);
}

/* Compute and set the parity of each byte in the key */
static void compute_byte_parity(void *k)
{
	unsigned char *x = k;
	for (int i = 0; i < 8; i++) {
		if (!__builtin_parity(((unsigned int) x[i] & 0xfe)))
			x[i] |= (unsigned char) 0x01;
		else
			x[i] &= (unsigned char) 0xfe;
	}
}

static uint64_t maybe_flip_key(uint64_t k)
{
	#if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	return __builtin_bswap64(k);
	#elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
	return k;
	#endif
}

/* Generate a random DES key when the program is started */
static void fill_random_des_key(void *k)
{
	#ifndef TEST
		int r = 24, fd = open("/dev/urandom", O_RDONLY);
		while (r -= read(fd, k, r))
			k += r;
		close(fd);
	#else
		bzero(k, 24);
	#endif
}

/* Originally, I had each thread generate their own random keys and use those
 * to test, but this is suboptimal because the ranges that each thread is testing
 * could easily overlap eventually, especially on a system with a lot of CPU's.
 * The solution is to use a global key that each thread increments as it uses it.
 * Generating the key schedule takes a long time, so it's much more efficient to
 * copy the shared key to a thread-local variable, which is then used to generate
 * the key. The overhead of this locking is not measureable on my laptop with a
 * thread for each of 4 physical CPU's. */
static void next_3des_key(DES_cblock_aligned *k, DES_cblock_aligned *t, DES_key_schedule *s, int *lock)
{
	int i = 0;

	take_spinlock(lock);
	do {
		k[i].num64 |= 0x0101010101010101UL; /* DES ignores parity bit, don't test same key twice */
		t[i].num64 = ++k[i].num64;
		if (t[i++].num64)
			break;
	} while (i < 3);
	release_spinlock(lock);

	i--;
	do {
		DES_set_key_unchecked(&t[i].des, &s[i]);
	} while (--i >= 0);
}

static void initialize_thread(DES_cblock_aligned *tmp, DES_cblock_aligned *src, DES_key_schedule *s, int *lock)
{
	take_spinlock(lock);
	
	/* Get a unique bottom third */
	src[0].num64 |= 0x0101010101010101UL;
	tmp[0].num64 = ++src[0].num64;

	/* ... and just copy the rest */
	for (int i = 1; i < 3; i++)
		tmp[i].num64 = src[i].num64;
	
	release_spinlock(lock);

	/* Expand the initial keys */
	for (int i = 0; i < 3; i++)
 		DES_set_key_unchecked(&tmp[i].des, &s[i]);
}

static int find_key(uint64_t *n, void *key_block)
{
	int *key_lock = key_block + 24;

	/* Our thread-local keys and key schedules */
	DES_key_schedule s[3];
	DES_cblock_aligned tmp[3];
	DES_cblock_aligned output;

	initialize_thread(tmp, key_block, s, key_lock);	
	while (1) {
		DES_ecb3_encrypt(&ciphertext.des, &output.des, &s[0], &s[1], &s[2], DES_DECRYPT);

		/* strncmp() is expensive - check the first four characters first. */
		if (output.num32 == plain_as_int)
			if (strncmp((char*) output.des, plaintext, 6) == 0)
				break;
	
		/* Increment the global counter and try the next key */
		next_3des_key(key_block, tmp, s, key_lock);
		(*n)++;

	}

	/* Found it: Compute the proper parity and print for the user */
	for (int i = 0; i < 3; i++)	
		compute_byte_parity(&tmp[i]);
	printf("Found key: %016" PRIx64 " %016" PRIx64 " %016" PRIx64 "\n", maybe_flip_key(tmp[0].num64), maybe_flip_key(tmp[1].num64), maybe_flip_key(tmp[2].num64));
	printf("Yields: (%hhx %hhx %hhx %hhx %hhx %hhx %hhx %hhx)\n", output.des[0], output.des[1],
			output.des[2], output.des[3], output.des[4], output.des[5], output.des[6], output.des[7]);
	return 0;
}

int main(int nargs, char **args)
{
	uint64_t *n = mmap(NULL, 8, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, 0, 0);
	unsigned char *k = mmap(NULL, 32, PROT_READ|PROT_WRITE, MAP_ANON|MAP_SHARED, 0, 0);
	uint64_t tmp = 0;
	struct timespec then, now;

	if (nargs != 2)
		return printf("Specify the number of threads to spawn as a single argument\n");

	fill_random_des_key(k);
	clock_gettime(CLOCK_MONOTONIC_COARSE, &then);
	for (int i = 0; i <= atoi(args[1]) - 1; i++)
		if (!fork())
			return find_key(n, k);

	while(1) {
		if ((*n >> 26) > tmp) {
			clock_gettime(CLOCK_MONOTONIC_COARSE, &now);
			printf("%" PRIu64 " keys tried in %" PRIu64 " seconds (%" PRIu64 " per second)\n", *n, (uint64_t) now.tv_sec - then.tv_sec, *n / (now.tv_sec - then.tv_sec));
			tmp++;
		}
		sleep(5);
	}
}
