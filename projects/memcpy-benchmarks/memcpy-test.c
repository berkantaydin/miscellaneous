#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>
#include <sys/mman.h>
#include <sched.h>

#define CACHE_LINE_BYTES 64

static inline void flush_cache_line(const void *addr)
{
	asm volatile ("clflush (%0);" :: "r" (addr) : "memory");
}

static void dump_caches(const void *a, const void *b, size_t n)
{
	size_t i;
	const char *aa = a;
	const char *bb = b;

	n /= CACHE_LINE_BYTES;
	for (i = 0; i < n; i++) {
		flush_cache_line(aa + i * CACHE_LINE_BYTES);
		flush_cache_line(bb + i * CACHE_LINE_BYTES);
	}
	asm volatile ("mfence;" ::: "memory");
}

static void *naive_incaddr(void *dst, const void *src, size_t n)
{
	void *ret = dst;
	unsigned long tmp = 0;

	while (n--) {
		asm ("movb (%0),%b1; movb %b1,(%2);" ::
			"r" (src), "r" (tmp), "r" (dst) :
			"memory");
		src++;
		dst++;
	}

	return ret;
}

static void *naive_effaddr(void *dst, const void *src, size_t n)
{
	unsigned long tmp = 0;
	size_t i;

	for (i = 0; i < n; i++)
		asm ("movb (%0,%3,1),%b1; movb %b1,(%2,%3,1);" ::
			"r" (src), "r" (tmp), "r" (dst), "r" (i) :
			"memory");

	return dst;
}

static void *erms(void *dst, const void *src, size_t n)
{
	asm ("rep movsb;" :: "S" (dst), "D" (src), "c" (n) : "memory","cc");
	return dst;
}

static void *sse_nocache64(void *dst, const void *src, size_t n)
{
	unsigned long tmp = 0;
	size_t i;

	n /= 8;
	for (i = 0; i < n; i++)
		asm ("movq (%0,%3,8),%1; movntiq %1,(%2,%3,8);" ::
				"r" (src), "r" (tmp), "r" (dst), "r" (i) :
				"memory");

	return dst;
}

static void *sse_aligned128(void *dst, const void *src, size_t n)
{
	double tmp = 0;
	size_t i;

	n /= 16;
	for (i = 0; i < n; i++) {
		asm ("movdqa (%0),%1; movdqa %1,(%0);" ::
			"r" (src), "x" (tmp), "r" (dst) : "memory");
		src += 16;
		dst += 16;
	}

	return dst;
}

static void *avx_aligned256(void *dst, const void *src, size_t n)
{
	double tmp = 0;
	size_t i;

	n /= 32;
	for (i = 0; i < n; i++) {
		asm ("vmovdqa (%0),%1; vmovdqa %1,(%0);" ::
			"r" (src), "x" (tmp), "r" (dst) : "memory");
		src += 32;
		dst += 32;
	}

	return dst;
}

struct memcpy_func {
	void *(*func)(void*, const void*, size_t);
	char *name;
};

#define MEMCPY_FUNC(fn) {.func = fn, .name = #fn,}
static struct memcpy_func funcs[] = {
	MEMCPY_FUNC(naive_incaddr),
	MEMCPY_FUNC(naive_effaddr),
	MEMCPY_FUNC(sse_nocache64),
	MEMCPY_FUNC(sse_aligned128),
	MEMCPY_FUNC(avx_aligned256),
	MEMCPY_FUNC(erms),
	MEMCPY_FUNC(memcpy),
	MEMCPY_FUNC(NULL),
};

static void *alloc_buffer(size_t size)
{
	void *ret = mmap(NULL, size, PROT_READ|PROT_WRITE,
			MAP_ANON|MAP_PRIVATE|MAP_POPULATE, 0, 0);

	if (ret == MAP_FAILED) {
		puts("Couldn't allocate buffer!");
		abort();
	}

	return ret;
}


static int monopolize_cpu(void)
{
	int ret;
	cpu_set_t cpumask;
	struct sched_param sp = {
		.sched_priority = sched_get_priority_max(SCHED_FIFO),
	};

	CPU_ZERO(&cpumask);

	ret = sched_getcpu();
	CPU_SET(ret, &cpumask);

	ret = sched_setaffinity(0, sizeof(cpumask), &cpumask);
	if (ret == -1)
		return -1;

	return sched_setscheduler(0, SCHED_FIFO, &sp);
}

void run_tests(int max_order, long count)
{
	int order, i, j;
	char *src_buf, *dst_buf;
	struct memcpy_func *cur;
	struct timespec then, now;
	long nsec_elapsed, buffer_size, total_copied;
	double ns_per_kbyte;

	buffer_size = 1L << max_order;

	src_buf = alloc_buffer(buffer_size);
	dst_buf = alloc_buffer(buffer_size);

	printf("%20s:", "ORDER");
	for (order = 4; order <= max_order; order++)
		printf(" %-5d", order);
	puts("");

	for (i = 0; (cur = &funcs[i]) && cur->func; i++) {
		printf("%-20s:", cur->name);

		for (order = 4; order <= max_order; order++) {
			dump_caches(src_buf, dst_buf, 1UL << order);

			clock_gettime(CLOCK_MONOTONIC_RAW, &then);
			for (j = 0; j < count; j++) {
				cur->func(src_buf, dst_buf, 1UL << order);
				cur->func(dst_buf, src_buf, 1UL << order);
			}
			clock_gettime(CLOCK_MONOTONIC_RAW, &now);

			nsec_elapsed = (now.tv_sec - then.tv_sec) * 1000000000L;
			nsec_elapsed += (now.tv_nsec - then.tv_nsec);

			total_copied = (1L << order) * count * 2;
			ns_per_kbyte = (double)nsec_elapsed / total_copied * 1000.0;
			printf(" %05.0f", ns_per_kbyte);
		}

		puts("");
	}

	munmap(src_buf, buffer_size);
	munmap(dst_buf, buffer_size);
}

int main(void)
{
	if (monopolize_cpu())
		printf("WARNING: Couldn't hog CPU: %m\n");

	run_tests(25, 200);
	return 0;
}
