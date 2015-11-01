#pragma once
struct bfi {
	unsigned long *n;
	int wlen;
	int allocated_wlen;
	int sign;
};

struct bfi *new_bfi(int bitlen);
void free_bfi(struct bfi *n);
void bfi_inplace_copy(struct bfi *dst, struct bfi *src);
struct bfi *new_bfi_copyof(struct bfi *src);
void xchg_bfi(struct bfi *a, struct bfi *b);
void try_to_shrink_bfi(struct bfi *p);
void extend_bfi(struct bfi *p, int bitlen_want);
void print_bfi(struct bfi *p);
int bfi_bit_set(struct bfi *n, int bit);
int index_of_most_sig_bit(struct bfi *x);
void do_bfi_multiply(struct bfi **out, struct bfi *a, struct bfi *b);
int bfi_gte(struct bfi *a, struct bfi *b);
int bfi_gt(struct bfi *a, struct bfi *b);
int bfi_eq(struct bfi *a, struct bfi *b);
void do_bfi_add(struct bfi *a, struct bfi *b);
void do_bfi_sub(struct bfi *a, struct bfi *b);
void do_bfi_inc(struct bfi *n);
void do_bfi_dec(struct bfi *n);
void bfi_shr(struct bfi *x);
void bfi_shl(struct bfi *x);
void bfi_multiple_shl(struct bfi *x, int n);
int bfi_is_zero(struct bfi *x);
int bfi_is_one(struct bfi *x);
void do_bfi_mod(struct bfi *n, struct bfi *mod);
void do_bfi_div(struct bfi *dividend, struct bfi *divisor, struct bfi **quotient, struct bfi **remainder);
struct bfi *bfi_gcd(struct bfi *a, struct bfi *b);
struct bfi *do_bfi_mod_exp(struct bfi *base, struct bfi *exp, struct bfi *mod);
int bfi_is_divby_three(struct bfi *n);
