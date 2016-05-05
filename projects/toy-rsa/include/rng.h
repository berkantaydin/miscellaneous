#pragma once

#ifdef CONSTRNG
#define ENTROPY_SOURCE "/dev/zero"
#else
#define ENTROPY_SOURCE "/dev/random"
#endif

void rng_fill_mem(void *mem, int len);
void init_rng(void);
void free_rng(void);
