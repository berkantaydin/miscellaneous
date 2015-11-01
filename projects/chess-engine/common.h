#pragma once
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#ifdef DEBUG
#define debug(...) \
do { \
	printf("DEBUG: "__VA_ARGS__); \
	fflush(stdout); \
} while (0)
#else
#define debug(...) \
do { \
} while (0)
#endif

#define fatal(...) \
do { \
	printf("FATAL: "__VA_ARGS__); \
	abort(); \
} while (0)

#if __CHAR_BIT__ != 8
	#error "This code assumes 8-bit bytes. Sorry."
#endif

#if __SIZEOF_LONG__ == 4
	#define LONG_BITS 32
	#define LONG_DIVSHIFT 5
	#define LONG_MODMASK 0x1f
#elif __SIZEOF_LONG__ == 8
	#define LONG_BITS 64
	#define LONG_DIVSHIFT 6
	#define LONG_MODMASK 0x3f
#else
	#error "Your machine has a weird long integer type. You can't build this."
#endif

#ifndef CLOCK_MONOTONIC_COARSE
	#ifndef CLOCK_MONOTONIC
		#warn "Your kernel is prehistoric: timekeeping functions may be inaccurate"
		#define CLOCK_MONOTONIC_COARSE CLOCK_REALTIME
	#else
		#warn "Your kernel is ancient: timekeeping functions will cause significant slowdown"
		#define CLOCK_MONOTONIC_COARSE CLOCK_MONOTONIC
	#endif
#endif

/* For places where we don't want to handle -ENOMEM */
static inline __attribute__((always_inline)) void *ecalloc(int num_elem, int elem_size)
{
	void *ret = calloc(num_elem, elem_size);
	if (!ret)
		fatal("-ENOMEM!\n");
	return ret;
}

static inline __attribute__((always_inline)) struct timespec *start_timer(void)
{
	#ifdef DEBUG
	struct timespec *r = ecalloc(1, sizeof(*r));
	clock_gettime(CLOCK_MONOTONIC_COARSE, r);
	return r;
	#else
	return NULL;
	#endif
}

static inline __attribute__((always_inline)) long get_timer_and_free(struct timespec *t)
{
	#ifdef DEBUG
	struct timespec tmp;
	clock_gettime(CLOCK_MONOTONIC_COARSE, &tmp);
	long ret = (tmp.tv_sec - t->tv_sec) * 1000L + (tmp.tv_nsec - t->tv_nsec) / 1000000L;
	free(t);
	return ret;
	#else
	return 0;
	#endif
}
