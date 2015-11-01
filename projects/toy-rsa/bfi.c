/* bfi.c: Amazingly suboptimal portable bignum library
 *
 * Copyright (C) 2014 Calvin Owens
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of version 2 only of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THIS SOFTWARE.
 *
 * BFI's ("Big fucking integers") are stored as arrays of 'unsigned long',
 * which is generally the underlying word size of the machine. On some archs
 * this may not be optimal, but it probably is on most.
 *
 * BFI's are sign-magnitude. The underlying array of long's is always treated
 * as unsigned, and the add/sub functions actually invoke each other depending
 * on the signs of the arguments.
 *
 * The only large constraint is that we can only run on architectures with
 * an expanding multiply instruction. Every architecture Linux runs on has
 * one, AFAIK.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

struct bfi {
	unsigned long *n;
	int wlen;
	int allocated_wlen;
	int sign;
};

void *ecalloc(int elem, int sz)
{
       void *ret = calloc(elem, sz);
       if (!ret)
               fatal("-ENOMEM!\n");
       return ret;
}

static struct bfi *internal_new_bfi(int wlen)
{
	struct bfi *ret = ecalloc(1, sizeof(struct bfi));
	int w_alloc = wlen;

	/* Round up allocations to 8-byte chunks */
	if (w_alloc & 0x07) {
		w_alloc >>= 3;
		w_alloc++;
		w_alloc <<= 3;
	}

	ret->n = ecalloc(w_alloc, sizeof(unsigned long));
	ret->wlen = wlen;
	ret->allocated_wlen = w_alloc;
	ret->sign = 0;

	return ret;
}

struct bfi *new_bfi(int bitlen)
{
	return internal_new_bfi(bitlen >> ULONG_BITSHIFT);
}

void free_bfi(struct bfi *n)
{
	free(n->n);
	free(n);
}

static struct bfi *alloc_bfi_for_multiply(struct bfi *a, struct bfi *b)
{
	return internal_new_bfi(a->wlen + b->wlen);
}

/* Copy one BFI to another without allocating memory */
void bfi_inplace_copy(struct bfi *dst, struct bfi *src)
{
	if (dst->allocated_wlen < src->wlen)
		fatal("Inplace copies should not need to allocate!\n");

	dst->wlen = src->wlen;
	dst->sign = src->sign;
	for (int i = 0; i < dst->allocated_wlen; i++)
		dst->n[i] = (i < src->wlen) ? src->n[i] : 0;
}

/* Allocate a new BFI as a copy of another */
struct bfi *new_bfi_copyof(struct bfi *src)
{
	struct bfi *ret = internal_new_bfi(src->wlen);
	bfi_inplace_copy(ret, src);
	return ret;
}

/* Exchange the contents of one BFI with another
 * Obviously, *much* faster than making a copy */
void xchg_bfi(struct bfi *a, struct bfi *b)
{
	unsigned long *tmp = a->n;
	int tmp1 = a->wlen;
	int tmp2 = a->allocated_wlen;
	int tmp3 = a->sign;

	a->n = b->n;
	a->wlen = b->wlen;
	a->allocated_wlen = b->allocated_wlen;
	a->sign = b->sign;

	b->n = tmp;
	b->wlen = tmp1;
	b->allocated_wlen = tmp2;
	b->sign = tmp3;
}

/* Remove most significant zero blocks from active portion of BFI */
void try_to_shrink_bfi(struct bfi *p)
{
	for (int i = p->wlen - 1; i >= 1; i--) {
		if (!p->n[i]) {
			p->wlen--;
		}
		else
			break;
	}
}

/* Make the BFI that long. May allocate memory */
static void internal_extend_bfi(struct bfi *p, int wlen_want)
{
	int wlen_to_alloc = wlen_want;
	unsigned long *new = NULL;

	if (wlen_want <= p->allocated_wlen) {
		p->wlen = wlen_want;
		return;
	}

	if (wlen_want & 0x07) {
		wlen_to_alloc >>= 3;
		wlen_to_alloc++;
		wlen_to_alloc <<= 3;
	}

	new = realloc(p->n, sizeof(unsigned long) * wlen_to_alloc);
	if (!new)
		fatal("-ENOMEM!\n");

	for (int i = p->allocated_wlen; i < wlen_to_alloc; i++)
		new[i] = 0;

	p->n = new;
	p->wlen = wlen_want;
	p->allocated_wlen = wlen_to_alloc;
}

void extend_bfi(struct bfi *p, int bitlen_want)
{
	internal_extend_bfi(p, bitlen_want >> ULONG_BITSHIFT);
}

/* Prinf a BFI to the screen */
void print_bfi(struct bfi *p)
{
	printf("[%d] ",p->sign);
	for (int i = p->wlen - 1; i >= 0; i--) {
		#if ULONG_BITS == 64
			printf("%016lx",p->n[i]);
		#elif ULONG_BITS == 32
			printf("%08lx",p->n[i]);
		#endif
	}
	printf("\n");
}

/* Return 1 if bit @bit is set in the BFI, 0 otherwise */
int bfi_bit_set(struct bfi *n, int bit)
{
	return (n->n[bit >> ULONG_BITSHIFT] & (unsigned long) ((unsigned long) 1 << ((unsigned long) bit & (unsigned long) ULONG_BITMASK))) ? 1 : 0;
}

/* Return the index of the most significant bit in the BFI */
int index_of_most_sig_bit(struct bfi *x)
{
	for (int i = x->wlen - 1; i >= 0; i--) {
		if (x->n[i])
			return (ULONG_BITCOUNT - __builtin_clzl(x->n[i])) + (i << ULONG_BITSHIFT);
	}
	return 0;
}

#if ULONG_BITS == 64
#define ODD_DIV3_CONST 0x5555555555555555UL
#define EVEN_DIV3_CONST 0xaaaaaaaaaaaaaaaaUL
static unsigned long nmod3[32] = {	0x9249249249249249UL,0x4924924924924924UL,0x2492492492492492UL,
					0x9249249249249249UL,0x4924924924924924UL,0x2492492492492492UL,
					0x9249249249249249UL,0x4924924924924924UL,0x2492492492492492UL,
					0x9249249249249249UL,0x4924924924924924UL,0x2492492492492492UL,
					0x9249249249249249UL,0x4924924924924924UL,0x2492492492492492UL,
					0x9249249249249249UL,0x4924924924924924UL,0x2492492492492492UL,
					0x9249249249249249UL,0x4924924924924924UL,0x2492492492492492UL,
					0x9249249249249249UL,0x4924924924924924UL,0x2492492492492492UL,
					0x9249249249249249UL,0x4924924924924924UL,0x2492492492492492UL,
					0x9249249249249249UL,0x4924924924924924UL,0x2492492492492492UL,
					0x9249249249249249UL,0x4924924924924924UL};
static struct bfi mod3 = {
	.n = (unsigned long*) &nmod3,
	.wlen = 32,
	.allocated_wlen = 32,
	.sign = 0
};
#elif ULONG_BITS == 32
#define ODD_DIV3_CONST 0x55555555UL
#define EVEN_DIV3_CONST 0xaaaaaaaaUL
static unsigned long nmod3[64] = {	0x92492492UL,0x49249249UL,0x49249249UL,0x24924924UL,0x24924924UL,0x92492492UL,
					0x92492492UL,0x49249249UL,0x49249249UL,0x24924924UL,0x24924924UL,0x92492492UL,
					0x92492492UL,0x49249249UL,0x49249249UL,0x24924924UL,0x24924924UL,0x92492492UL,
					0x92492492UL,0x49249249UL,0x49249249UL,0x24924924UL,0x24924924UL,0x92492492UL,
					0x92492492UL,0x49249249UL,0x49249249UL,0x24924924UL,0x24924924UL,0x92492492UL,
					0x92492492UL,0x49249249UL,0x49249249UL,0x24924924UL,0x24924924UL,0x92492492UL,
					0x92492492UL,0x49249249UL,0x49249249UL,0x24924924UL,0x24924924UL,0x92492492UL,
					0x92492492UL,0x49249249UL,0x49249249UL,0x24924924UL,0x24924924UL,0x92492492UL,
					0x92492492UL,0x49249249UL,0x49249249UL,0x24924924UL,0x24924924UL,0x92492492UL,
					0x92492492UL,0x49249249UL,0x49249249UL,0x24924924UL,0x24924924UL,0x92492492UL,
					0x92492492UL,0x49249249UL,0x49249249UL,0x24924924UL};
static struct bfi mod3 = {
	.n = (unsigned long*) &nmod3,
	.wlen = 64,
	.allocated_wlen = 64,
	.sign = 0
};
#endif

/* Quickly determine if a BFI is divisible by 3
 * This is a big win, since we can eliminate half of non-prime odd integers
 * without having to do the fermat primality test, which is expensive.
 * XXX: Maximum integer size here is 4096 bits */
int bfi_is_divby_three(struct bfi *n)
{
	int odd = 0;
	int even = 0;
	for (int i = 0; i < n->wlen; i++) {
		odd += __builtin_popcountl(n->n[i] & ODD_DIV3_CONST);
		even += __builtin_popcountl(n->n[i] & EVEN_DIV3_CONST);
	}

	return bfi_bit_set(&mod3, odd > even ? odd - even : even - odd);
}

/* Addition of two BFI's */
static void __bfi_add(struct bfi *a, struct bfi *b)
{
	unsigned long tmp;

	if (a->wlen < b->wlen)
		internal_extend_bfi(a,b->wlen);

	for (int i = 0; i < a->wlen; i++) {
		tmp = a->n[i];
		a->n[i] += b->n[i];
		if (a->n[i] < tmp) {
			for (int j = i + 1; j < a->wlen; j++) {
				tmp = a->n[j];
				if (++a->n[j] > tmp)
					break;
			}
		}
	}
}

/* Subtraction of BFI's */
static void __bfi_sub(struct bfi *a, struct bfi *b)
{
	unsigned long tmp;

	try_to_shrink_bfi(a);
	try_to_shrink_bfi(b);

	if (a->wlen < b->wlen)
		internal_extend_bfi(a,b->wlen);

	for (int i = 0; i < b->wlen; i++) {
		tmp = a->n[i];
		a->n[i] -= b->n[i];
		if (tmp < a->n[i]) {
			for (int j = i + 1; j < a->wlen; j++) {
				tmp = a->n[j];
				a->n[j]--;
				if (tmp > a->n[j])
					break;
			}
		}
	}
}

/* Subtract @a from @b, store result in @a, don't touch @b */
static void __bfi_inv_sub(struct bfi *a, struct bfi *b, int sign)
{
	struct bfi *tmp = new_bfi_copyof(b);
	tmp->sign = sign;
	__bfi_sub(tmp,a);
	xchg_bfi(a,tmp);
	free_bfi(tmp);
}

/* Return 1 if @a > @b, 2 if @a == @b, 0 otherwise */
int bfi_gte(struct bfi *a, struct bfi *b)
{
	try_to_shrink_bfi(a);
	try_to_shrink_bfi(b);

	if (a->wlen > b->wlen) return 1;
	if (b->wlen > a->wlen) return 0;

	int asig = index_of_most_sig_bit(a);
	int bsig = index_of_most_sig_bit(b);

	if (asig > bsig) return 1;
	if (bsig > asig) return 0;

	for (int i = a->wlen - 1; i >= 0; i--) {
		if (a->n[i] > b->n[i])
			return 1;
		if (a->n[i] < b->n[i])
			return 0;
	}

	return 2; /* a == b */
}

/* Return 1 if @a > @b, 0 otherwise */
int bfi_gt(struct bfi *a, struct bfi *b)
{
	int ret = bfi_gte(a,b);
	if (ret == 2)
		return 0;
	else
		return ret;
}

/* Return 1 if @a == @b, 0 otherwise */
int bfi_eq(struct bfi *a, struct bfi *b)
{
	try_to_shrink_bfi(a);
	try_to_shrink_bfi(b);

	if (a->wlen != b->wlen)
		return 0;

	if (index_of_most_sig_bit(a) != index_of_most_sig_bit(b))
		return 0;

	for (int i = 0; i < a->wlen; i++)
		if (a->n[i] != b->n[i])
			return 0;
	return 1;
}

/* Return 1 if @x is zero, 0 otherwise */
int bfi_is_zero(struct bfi *x)
{
	for (int i = 0; i < x->wlen; i++)
		if (x->n[i])
			return 0;
	return 1;
}

/* Return 1 if @x == 1, 0 otherwise */
int bfi_is_one(struct bfi *x)
{
	if (x->n[0] != 1)
		return 0;

	for (int i = 1; i < x->wlen; i++)
		if (x->n[i])
			return 0;
	return 1;
}

struct exmulres {
	unsigned long hi;
	unsigned long lo;
};

/* Only use this for multiplication, since that can't overflow */
static inline void carry_on_my_wayward_son(unsigned long *x, unsigned long a)
{
	unsigned long tmp = *x;
	*x += a;
	while (*x++ < tmp) {
		tmp = *x;
		*x += 1;
	}
}

/* XXX: CPU dependent - do a single expanding multiplication
 * We always treat the integers as unsigned */
static inline struct exmulres do_one_expanding_multiply(unsigned long a, unsigned long b)
{
	struct exmulres res;
	#if defined(__x86_64__)
		asm ("movq %0,%%rax; mulq %1; movq %%rdx,%1; movq %%rax,%0"
		: "=r" (res.lo), "=r" (res.hi) : "0" (a), "1" (b) : "%rax","%rdx");
	#elif defined(__i386__)
		asm ("movl %0,%%eax; mull %1; movl %%edx,%1; movl %%eax,%0"
		: "=r" (res.lo), "=r" (res.hi) : "0" (a), "1" (b) : "%eax","%edx");
	#elif defined(__arm__)
		asm ("umull %0,%1,%2,%3"
		: "=r" (res.lo), "=r" (res.hi) : "r" (a), "r" (b) :);
	#else
		#error "No expanding multiply assembly has been written for your architecture"
	#endif
	return res;
}

/* Multiply @a by @b, and put the result at @out */
void do_bfi_multiply(struct bfi **out, struct bfi *a, struct bfi *b)
{
	struct exmulres r;
	struct bfi *res;

	int free_a = (a == *out) ? 1 : 0;
	int free_b = ((b == *out) && (b != a)) ? 1 : 0;

	try_to_shrink_bfi(a);
	try_to_shrink_bfi(b);
	res = alloc_bfi_for_multiply(a,b);
	for (int i = 0; i < b->wlen; i++) {
		for (int j = 0; j < a->wlen; j++) {
			r = do_one_expanding_multiply(a->n[j],b->n[i]);
			carry_on_my_wayward_son(&res->n[i+j+1],r.hi);
			carry_on_my_wayward_son(&res->n[i+j],r.lo);
		}
	}
	try_to_shrink_bfi(res);

	if (a->sign ^ b->sign)
		res->sign = 1;
	if (free_a)
		free_bfi(a);
	if (free_b)
		free_bfi(b);

	*out = res;
}

/* Addition for BFI's
 * May actually do subtraction, depending on the signs of the arguments */
void do_bfi_add(struct bfi *a, struct bfi *b)
{
	int cmp = bfi_gte(b,a);
	int signs = (a->sign << 1) | (b->sign);

	switch (signs) {
	case 0:
	case 3:
		return __bfi_add(a,b);
	case 1:
		switch (cmp) {
		case 1:
			return __bfi_inv_sub(a,b,1);
		case 2:
		case 0:
			return __bfi_sub(a,b);
		}
	case 2:
		switch (cmp) {
		case 1:
			return __bfi_inv_sub(a,b,0);
		case 2:
		case 0:
			return __bfi_sub(a,b);
		}
	}
}

/* Subtraction for BFI's
 * May actually do addition, depending on signs of the arguments */
void do_bfi_sub(struct bfi *a, struct bfi *b)
{
	int cmp = bfi_gte(b,a);
	int signs = (a->sign << 1) | (b->sign);

	switch (signs) {
	case 0:
		switch (cmp) {
		case 1:
			return __bfi_inv_sub(a,b,1);
		case 2:
		case 0: return __bfi_sub(a,b);
		}
	case 1: 
		return __bfi_add(a,b);
	case 2: 
		return __bfi_add(a,b);
	case 3:
		switch (cmp) {
		case 0: 
			return __bfi_sub(a,b);
		case 1:
			return __bfi_inv_sub(a,b,0);
		case 2: 
			a->sign = 0;
			return __bfi_sub(a,b);
		}
	}
}

/* Increment a BFI by 1 */
void do_bfi_inc(struct bfi *n)
{
	int i = 0;
	while (!(++n->n[i++]))
		;
}

/* Decrement a BFI by 1
 * XXX: This blows up if you call it on 0 */
void do_bfi_dec(struct bfi *n)
{
	int i = 0;
	while (!(n->n[i++]--))
		;
}

/* Shift an entire BFI right one bit */
void bfi_shr(struct bfi *x)
{
	unsigned long tmp[2] = {0,0};
	for (int i = x->wlen - 1; i >= 0; i--) {
		tmp[i & 1] = (x->n[i] & 1) << ULONG_BITCOUNT;
		x->n[i] >>= 1;
		x->n[i] |= tmp[(i & 1) ^ 1];
	}
}

/* Shift an entire BFI left by one bit */
void bfi_shl(struct bfi *x)
{
	unsigned long tmp[2] = {0,0};
	for (int i = 0; i < x->wlen; i++) {
		tmp[i & 1] = (x->n[i] & ULONG_TOPBITMASK) >> ULONG_BITCOUNT;
		x->n[i] <<= 1;
		x->n[i] |= tmp[(i & 1) ^ 1];
	}
}

/* Shift a BFI left by @n bits
 * Copies whole qwords if @n > 64, which is a big win */
void bfi_multiple_shl(struct bfi *x, int n)
{
	int w_shift = n >> ULONG_BITSHIFT;
	if (w_shift) {
		for (int i = x->wlen - 1; i >= w_shift; i--)
			x->n[i] = x->n[i - w_shift];
		for (int i = w_shift - 1; i >= 0; i--)
			x->n[i] = 0;
		n &= ULONG_BITMASK;
	}

	for (int i = 0; i < n; i++)
		bfi_shl(x);
}

/* Reduce @n modulo @mod */
void do_bfi_mod(struct bfi *n, struct bfi *mod)
{
	int n_sig, mod_sig, s;

	if (bfi_gt(mod,n))
		return;

	try_to_shrink_bfi(n);
	if (mod->wlen < n->wlen)
		internal_extend_bfi(mod,n->wlen);

	n_sig = index_of_most_sig_bit(n);
	mod_sig = index_of_most_sig_bit(mod);
	s = n_sig - mod_sig;

	bfi_multiple_shl(mod,s);

	while (s) {
		if (bfi_gte(n,mod)) {
			__bfi_sub(n,mod);
		} else {
			bfi_shr(mod);
			s--;
		}
	}

	if (bfi_gte(n,mod))
		__bfi_sub(n,mod);

	if (bfi_gte(n,mod))
		fatal("This should NEVER happen (modulo)\n");

	try_to_shrink_bfi(n);
}

/* Same as do_bfi_mod, with an additional shift and add to calculate the quotient */
void do_bfi_div(struct bfi *dividend, struct bfi *divisor, struct bfi **quotient, struct bfi **remainder)
{
	unsigned int a_sig, b_sig, s;
	struct bfi *a = new_bfi_copyof(dividend);
	struct bfi *b = new_bfi_copyof(divisor);
	struct bfi *tmp = internal_new_bfi(a->wlen);
	tmp->n[0] = 1;

	try_to_shrink_bfi(a);
	if (a->wlen < b->wlen)
		internal_extend_bfi(a,b->wlen);
	if (b->wlen < a->wlen)
		internal_extend_bfi(b,a->wlen);

	if (bfi_gt(divisor,dividend))
		fatal("Divisor cannot be larger than dividend\n");

	*quotient = internal_new_bfi(a->wlen);

	a_sig = index_of_most_sig_bit(a);
	b_sig = index_of_most_sig_bit(b);
	s = a_sig - b_sig;

	bfi_multiple_shl(b,s);
	bfi_multiple_shl(tmp,s);

	while (s) {
		if (bfi_gte(a,b)) {
			__bfi_sub(a,b);
			__bfi_add(*quotient,tmp);
		} else {
			bfi_shr(b);
			bfi_shr(tmp);
			s--;
		}
	}

	if (bfi_gte(a,b)) {
		__bfi_sub(a,b);
		__bfi_add(*quotient,tmp);
	}

	free_bfi(b);
	free_bfi(tmp);
	*remainder = a;
}

/* Euclidian algorithm: return gcd(a,b) */
struct bfi *bfi_gcd(struct bfi *a, struct bfi *b)
{
	struct bfi *ra = new_bfi_copyof(a);
	struct bfi *rb = new_bfi_copyof(b);
	struct bfi *t = internal_new_bfi(a->wlen);

	while(!bfi_is_zero(rb)) {
		xchg_bfi(t,rb);
		do_bfi_mod(ra,t);
		xchg_bfi(ra,rb);
		xchg_bfi(ra,t);
	}

	free_bfi(rb);
	free_bfi(t);
	return ra;
}

/* Return @base^@exp mod @mod */
struct bfi *do_bfi_mod_exp(struct bfi *base, struct bfi *exp, struct bfi *mod)
{
	struct bfi *res = internal_new_bfi(mod->wlen);
	signed int bit = index_of_most_sig_bit(exp);
	res->n[0] = 1;
	while (bit + 1) {
		do_bfi_multiply(&res,res,res);
		do_bfi_mod(res,mod);

		if (bfi_bit_set(exp,bit--)) {
			do_bfi_multiply(&res,res,base);
			do_bfi_mod(res,mod);
		}
	}

	return res;
}
