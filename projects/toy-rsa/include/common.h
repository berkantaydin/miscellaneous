#pragma once

#define fatal(args...) \
do { \
	printf(args); \
	abort(); \
} while (0)

#define min(x, y) ({							\
	typeof(x) _min1 = (x);						\
	typeof(y) _min2 = (y);						\
	(void) (&_min1 == &_min2);					\
	_min1 < _min2 ? _min1 : _min2; })

#define max(x, y) ({							\
	typeof(x) _max1 = (x);						\
	typeof(y) _max2 = (y);						\
	(void) (&_max1 == &_max2);					\
	_max1 > _max2 ? _max1 : _max2; })

#define xchg(x, y) do {	\
	typeof(x) _tmp;	\
	_tmp = x;	\
	x = y;		\
	y = _tmp;	\
} while (0)

#define __must_check __attribute__((warn_unused_result))

#ifdef CONSTRNG
#define ENTROPY_SOURCE "/dev/zero"
#else
#define ENTROPY_SOURCE "/dev/random"
#endif

#define PRINT_HEX_BLOB(x,n) \
do { \
	printf("0x"); \
	for (int i = 0; i < n; i++) \
		printf("%02hhx", (unsigned char) x[i]); \
	printf("\n"); \
} while(0)
