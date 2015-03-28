#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sched.h>

#define TEST_BUFFER_ORDER 30
#define NR_TEST_COPIES 10
#define TEST_BUFFER_SIZE ((1UL << TEST_BUFFER_ORDER))

#define TESTED_BYTES ((TEST_BUFFER_SIZE * NR_TEST_COPIES))

#define CACHE_LINE_BYTES 64
#define NR_CPUS 8

struct cache_line {
	char b[CACHE_LINE_BYTES];
} __attribute__((aligned(CACHE_LINE_BYTES)));

static inline void flush_cache_line(const void *addr)
{
	asm volatile ("clflush (%0);" :: "r" (addr) : "memory");
}

static void dump_caches(const void *a, const void *b, size_t n)
{
	size_t i;
	const struct cache_line *csrc = a;
	const struct cache_line *cdst = b;

	n /= sizeof(struct cache_line);
	for (i = 0; i < n; i++) {
		flush_cache_line(&cdst[i]);
		flush_cache_line(&csrc[i]);
	}
	asm volatile ("mfence;" ::: "memory");
}

static void *memcpy_erms(void *dst, const void *src, size_t n)
{
	asm volatile ("rep movsb;" :: "S" (dst), "D" (src), "c" (n) : "memory");
	return dst;
}

/*
 * XXX: This one assumes 8-byte alignment
 */
static void *memcpy_nocache(void *dst, const void *src, size_t n)
{
	asm volatile (	"shr $3,%2;"
			"1:\n"
			"movq -0x8(%1,%2,8),%%rdx;"
			"movntiq %%rdx,-0x8(%0,%2,8);"
			"decq %2;"
			"jnz 1b;"
			"sfence;" ::
			"r" (dst), "r" (src), "r" (n) : "%rdx");

	return dst;
}

struct memcpy_func {
	void *(*func)(void*,const void*,size_t);
	char *name;
};

#define MEMCPY_FUNC(fn, nm) {.func = fn, .name = nm,}
static struct memcpy_func funcs[] = {
	MEMCPY_FUNC(memcpy_nocache, "movnti"),
	MEMCPY_FUNC(memcpy_erms, "erms"),
	MEMCPY_FUNC(memcpy, "Glibc"),
};

static void *alloc_buffer(size_t size)
{
	void *ret = mmap(NULL, size, PROT_READ|PROT_WRITE, MAP_ANON|MAP_PRIVATE|MAP_POPULATE, 0, 0);

	if (ret == MAP_FAILED) {
		printf("Couldn't allocate buffers!\n");
		abort();
	}

	return ret;
}

static int become_realtime_sortof(void)
{
	int ret;
	cpu_set_t *cpumask;
	struct sched_param sp = {
		.sched_priority = sched_get_priority_max(SCHED_FIFO),
	};

	cpumask = CPU_ALLOC(NR_CPUS);
	if (!cpumask)
		return -1;

	CPU_ZERO(cpumask);
	ret = sched_getcpu();
	CPU_SET(ret, cpumask);
	ret = sched_setaffinity(0, CPU_ALLOC_SIZE(NR_CPUS), cpumask);
	if (ret == -1)
		return -1;

	return sched_setscheduler(0, SCHED_FIFO, &sp);
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

	if (become_realtime_sortof())
		printf("WARNING: Couldn't become realtime sortof: %m\n");

	nr_tests = sizeof(funcs) / sizeof(funcs[0]);
	printf("Performing %d tests, %.3fGiB!\n", nr_tests,
		TESTED_BYTES / (double) (1UL << 30));
	for (i = 0; i < nr_tests; i++) {
		cur = &funcs[i];

		/*
		 * Dump all testing area out of the cache.
		 */
		dump_caches(src_buf, dst_buf, TEST_BUFFER_SIZE);

		clock_gettime(CLOCK_MONOTONIC_RAW, &then);
		for (j = 0; j < NR_TEST_COPIES / 2; j++) {
			cur->func(src_buf, dst_buf, TEST_BUFFER_SIZE);
			cur->func(dst_buf, src_buf, TEST_BUFFER_SIZE);
		}
		clock_gettime(CLOCK_MONOTONIC_RAW, &now);

		nsec_elapsed = (now.tv_sec - then.tv_sec) * 1000000000L;
		nsec_elapsed += (now.tv_nsec - then.tv_nsec);
		printf("%s:\t%.9f seconds, %luus/MiB, %luns/page, %.3fns/cacheline\n",
			cur->name,
			nsec_elapsed / (double) 1000000000L,
			(nsec_elapsed / (TESTED_BYTES >> 20)) / 1000L,
			(nsec_elapsed / (TESTED_BYTES >> 12)),
			(nsec_elapsed / (double) (TESTED_BYTES / CACHE_LINE_BYTES)));
	}

	return 0;
}
