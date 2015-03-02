#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>

#define TEST_BUFFER_ORDER 12UL
#define TEST_BUFFER_SIZE ((1UL << TEST_BUFFER_ORDER))
#define NR_TEST_COPIES 10000000

#define INSTANTIATE_ZERO_FUNC(type) \
static void zero_##type(void *dst, size_t n) \
{ \
	type *d = dst; \
	n >>= __builtin_ffs(sizeof(type) / sizeof(char)) - 1; \
	while (--n) \
		d[n] = (type) 0UL; \
}

INSTANTIATE_ZERO_FUNC(char)
INSTANTIATE_ZERO_FUNC(short)
INSTANTIATE_ZERO_FUNC(int)
INSTANTIATE_ZERO_FUNC(long)
INSTANTIATE_ZERO_FUNC(__int128)
INSTANTIATE_ZERO_FUNC(__float128)

void zero_fast(void *dst, size_t n)
{
	asm volatile (	"xor %%eax,%%eax; movq %0,%%rdi; movq %1,%%rcx; rep stosb;" ::
			"r" (dst), "r" (n) :"%rax","%rdi","%rcx");
}

void zero_nocache(void *dst, size_t n)
{
	asm volatile (	"shr $3,%1;"
			"xorl %%edx,%%edx;"
			"1:\n"
			"movntiq %%rdx,-0x8(%0,%1,8);"
			"decq %1;"
			"jnz 1b;" ::
			"r" (dst), "r" (n) : "%rdx");
}

struct zero_func {
	void (*func)(void*,size_t);
	char *name;
};

static struct zero_func funcs[] = {
	{
		.func = zero_char,
		.name = "8-bit at-a-time",
	},
	{
		.func = zero_short,
		.name = "16-bit at-a-time",
	},
	{
		.func = zero_int,
		.name = "32-bit at-a-time",
	},
	{
		.func = zero_long,
		.name = "64-bit at-a-time",
	},
	{
		.func = zero___int128,
		.name = "128-bit at-a-time",
	},
	{
		.func = zero___float128,
		.name = "float128 at-a-time",
	},
	{
		.func = zero_fast,
		.name = "x86 string insns",
	},
	{
		.func = zero_nocache,
		.name = "x86 nocache64 write",
	},
};

static void *alloc_buffer(size_t size)
{
	void *ret = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE|MAP_POPULATE, 0, 0);

	if (ret == MAP_FAILED)
		abort();

	return ret;
}

int main(void)
{
	int i, j, nr_tests;
	long nsec_elapsed;
	char *dst_buf;
	struct zero_func *cur;
	struct timespec then, now;

	dst_buf = alloc_buffer(TEST_BUFFER_SIZE);

	nr_tests = sizeof(funcs) / sizeof(funcs[0]);
	printf("Zeroing %d pages, %d algos...\n", NR_TEST_COPIES, nr_tests);
	for (i = 0; i < nr_tests; i++) {
		cur = &funcs[i];

		printf("%s:\t", cur->name);
		clock_gettime(CLOCK_MONOTONIC_RAW, &then);
		for (j = 0; j < NR_TEST_COPIES; j++) {
			cur->func(dst_buf, TEST_BUFFER_SIZE);

			/* Don't let the write-combining instructions cheat */
			asm volatile("sfence");
		}
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);

		nsec_elapsed = (now.tv_sec - then.tv_sec) * 1000000000L;
		nsec_elapsed += (now.tv_nsec - then.tv_nsec);
		printf("%04lu nanoseconds\n", nsec_elapsed / NR_TEST_COPIES);
	}

	return 0;
}
