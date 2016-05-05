#pragma once

#include <limits.h>

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

#ifndef TESTCOUNT
#define TESTCOUNT LONG_MAX
#endif
