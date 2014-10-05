#pragma once
#define fatal(args...) \
do { \
	printf(args); \
	abort(); \
} while (0)

#ifdef DEBUG
#define debug(args...) \
do { \
	printf(args); \
} while (0)
#else
#define debug(args...) \
do { \
} while (0)
#endif

#ifdef URANDOM
#define ENTROPY_SOURCE "/dev/urandom"
#else
#define ENTROPY_SOURCE "/dev/random"
#endif

#if __SIZEOF_LONG__ == 8
	#define ULONG_BITS 64
	#define ULONG_BYTESHIFT 3
	#define ULONG_BITSHIFT 6
	#define ULONG_BITMASK 0x3f
	#define ULONG_BITCOUNT 63
	#define ULONGS_PER_AES_BLOCK 2
	#define ULONG_TOPBITMASK 0x8000000000000000UL
#elif __SIZEOF_LONG__ == 4
	#define ULONG_BITS 32
	#define ULONG_BYTESHIFT 2
	#define ULONG_BITSHIFT 5
	#define ULONG_BITMASK 0x1f
	#define ULONG_BITCOUNT 31
	#define ULONGS_PER_AES_BLOCK 4
	#define ULONG_TOPBITMASK 0x80000000UL
#else
	#error "Non 32/64-bit machines are unsupported"
#endif

#define PRINT_HEX_BLOB(x,n) \
do { \
	printf("0x"); \
	for (int i = 0; i < n; i++) \
		printf("%02hhx", (unsigned char) x[i]); \
	printf("\n"); \
} while(0)

void *ecalloc(int elem, int sz);
