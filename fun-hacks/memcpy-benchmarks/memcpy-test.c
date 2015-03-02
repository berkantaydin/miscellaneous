#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>

#define TEST_BUFFER_ORDER 30UL
#define TEST_BUFFER_SIZE ((1UL << TEST_BUFFER_ORDER))
#define NR_TEST_COPIES 10

#define INSTANTIATE_MEMCPY_FUNC(type) \
static void *memcpy_##type(void *dst, const void *src, size_t n) \
{ \
	volatile const type *s = src; \
	type *d = dst; \
	n >>= __builtin_ffs(sizeof(type) / sizeof(char)) - 1; \
	while (--n) \
		d[n] = s[n]; \
	return NULL; \
}

INSTANTIATE_MEMCPY_FUNC(char)
INSTANTIATE_MEMCPY_FUNC(short)
INSTANTIATE_MEMCPY_FUNC(int)
INSTANTIATE_MEMCPY_FUNC(long)
INSTANTIATE_MEMCPY_FUNC(__int128)
INSTANTIATE_MEMCPY_FUNC(__float128)

void *memcpy_fast(void *dst, const void *src, size_t n)
{
	asm volatile (	"movq %0,%%rsi; movq %1,%%rdi; movq %2,%%rcx; rep movsb;" ::
			"r" (dst), "r" (src), "r" (n) :"%rsi","%rdi","%rcx");
	return NULL;
}

void *memcpy_nocache(void *dst, const void *src, size_t n)
{
	asm volatile (	"shr $3,%2;"
			"1:\n"
			"movq -0x8(%1,%2,8),%%rdx;"
			"movntiq %%rdx,-0x8(%0,%2,8);"
			"decq %2;"
			"jnz 1b;"
			"sfence;" ::
			"r" (dst), "r" (src), "r" (n) : "%rdx");

	return NULL;
}

struct memcpy_func {
	void *(*func)(void*,const void*,size_t);
	char *name;
};

static struct memcpy_func funcs[] = {
	{
		.func = memcpy_char,
		.name = "8-bit at-a-time",
	},
	{
		.func = memcpy_short,
		.name = "16-bit at-a-time",
	},
	{
		.func = memcpy_int,
		.name = "32-bit at-a-time",
	},
	{
		.func = memcpy_long,
		.name = "64-bit at-a-time",
	},
	{
		.func = memcpy___int128,
		.name = "128-bit at-a-time",
	},
	{
		.func = memcpy___float128,
		.name = "float128 at-a-time",
	},
	{
		.func = memcpy_fast,
		.name = "x86 string insns",
	},
	{
		.func = memcpy_nocache,
		.name = "x86 nocache64 write",
	},
	{
		.func = memcpy,
		.name = "Glibc builtin memcpy",
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
	char *src_buf, *dst_buf;
	struct memcpy_func *cur;
	struct timespec then, now;

	src_buf = alloc_buffer(TEST_BUFFER_SIZE);
	dst_buf = alloc_buffer(TEST_BUFFER_SIZE);

	nr_tests = sizeof(funcs) / sizeof(funcs[0]);
	printf("Performing %d tests!\n", nr_tests);
	for (i = 0; i < nr_tests; i++) {
		cur = &funcs[i];

		printf("%s:\t", cur->name);
		clock_gettime(CLOCK_MONOTONIC_RAW, &then);
		for (j = 0; j < NR_TEST_COPIES / 2; j++) {
			cur->func(src_buf, dst_buf, TEST_BUFFER_SIZE);
			cur->func(dst_buf, src_buf, TEST_BUFFER_SIZE);
		}
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);

		nsec_elapsed = (now.tv_sec - then.tv_sec) * 1000000000L;
		nsec_elapsed += (now.tv_nsec - then.tv_nsec);
		printf("%.09lu nanoseconds\n", nsec_elapsed);
	}

	return 0;
}
