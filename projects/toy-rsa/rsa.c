/*
 * rsa.c: Simple RSA implementation
 *
 * Copyright (C) 2016 Calvin Owens <jcalvinowens@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify it under
 * the terms of the GNU General Public License as published by the Free Software
 * Foundation; version 2 only.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
 * details.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>

#include "include/common.h"
#include "include/rng.h"
#include "include/bfi.h"
#include "include/rsa.h"

struct rsa_key {
	struct bfi *exp;
	struct bfi *mod;
};

static void free_key(struct rsa_key *r)
{
	bfi_free(r->exp);
	bfi_free(r->mod);
	free(r);
}

/*
 * Return the modular multiplicative inverse of e mod tot
 */
static struct bfi *mod_inv(struct bfi *e, struct bfi *tot)
{
	struct bfi *a = bfi_copy(e);
	struct bfi *b = bfi_copy(tot);
	struct bfi *m = bfi_alloc(bfi_len(tot));
	struct bfi *x_last = bfi_alloc(bfi_len(tot));
	struct bfi *x = bfi_alloc(bfi_len(tot));
	struct bfi *q = NULL;
	struct bfi *r = NULL;
	struct bfi *tmp = NULL;

	bfi_raw(x)[0] = 1;
	while (!bfi_is_zero(a)) {
		q = bfi_divide(b, a, &r);

		xchg(m, x_last);
		tmp = bfi_multiply(q, x);
		bfi_sub(m, tmp);
		bfi_free(tmp);

		xchg(x_last, x);
		xchg(x, m);
		xchg(b, a);
		xchg(a, r);

		bfi_free(r);
		bfi_free(q);
	}

	if (bfi_sign(x_last))
		bfi_add(x_last, tot);

	bfi_modulo(x_last, tot);

	return x_last;
}

static int bfi_is_prime(struct bfi *n)
{
	struct bfi *rnd = bfi_alloc(bfi_len(n));
	struct bfi *nminusone = bfi_copy(n);
	struct bfi *res;
	int i, ret = 0;

	bfi_dec(nminusone);
	for (i = 0; i < 10; i++) {
		putchar('+');
		rng_fill_mem(bfi_raw(rnd), bfi_len(rnd) / CHAR_BIT);
		res = bfi_mod_exp(rnd, nminusone, n);
		if (!bfi_is_one(res))
			goto not_prime;
	}

	/*
	 * Probably prime
	 */
	ret = 1;

not_prime:
	bfi_free(rnd);
	bfi_free(res);
	bfi_free(nminusone);
	return ret;
}

static struct bfi *find_prime(unsigned int bits, struct bfi *e)
{
	struct bfi *prime = bfi_alloc(bits);
	struct bfi *tmp;

	puts("Searching for prime: ");

	while (1) {
		putchar('.');
		bfi_extend(prime, bits);
		rng_fill_mem(bfi_raw(prime), bits / CHAR_BIT);

		/*
		 * Don't waste time on even numbers.
		 */
		bfi_raw(prime)[0] |= 0x01UL;

		if (bfi_is_divby_three(prime))
			continue;

		bfi_dec(prime);
		tmp = bfi_gcd(prime, e);

		if (!bfi_is_one(tmp)) {
			putchar('!');
			bfi_free(tmp);
			continue;
		}

		bfi_free(tmp);
		bfi_inc(prime);

		if (bfi_is_prime(prime))
			break;
	}

	puts(" done!");
	return prime;
}

void rsa_generate_keypair(struct rsa_key **pub, struct rsa_key **priv, int bits)
{
	struct bfi *p, *q, *d, *mod, *tot, *tmp;
	struct bfi *e = bfi_alloc(64);

	bfi_raw(e)[0] = 65537;

	printf("Generating RSA key... ");
	p = find_prime(bits >> 1, e);
	q = find_prime(bits >> 1, e);
	mod = bfi_multiply(p, q);

	puts("Found a key!");
	printf("p: ");
	bfi_print(p);
	printf("q: ");
	bfi_print(q);

	bfi_dec(p);
	bfi_dec(q);
	tot = bfi_multiply(p, q);
	d = mod_inv(e, tot);

	printf("t: ");
	bfi_print(tot);

	printf("e: ");
	bfi_print(e);
	printf("m: ");
	bfi_print(mod);
	printf("d: ");
	bfi_print(d);

	bfi_free(tot);
	bfi_free(p);
	bfi_free(q);

	*pub = calloc(1, sizeof(**pub));
	*priv = calloc(1, sizeof(**priv));

	tmp = bfi_copy(mod);

	(*pub)->exp = e;
	(*pub)->mod = mod;
	(*priv)->mod = tmp;
	(*priv)->exp = d;
}

int rsa_cipher_test(int bits)
{
	struct rsa_key *pub,*priv;
	struct bfi *secret, *ciphertext, *decrypted;
	int ret = -1;

	rsa_generate_keypair(&pub, &priv, bits);

	secret = bfi_alloc(128);
	#if LONG_BIT == 64
	bfi_raw(secret)[0] = 0xbeefbeefbeefbeefUL;
	bfi_raw(secret)[1] = 0xbeefbeefbeefbeefUL;
	#elif LONG_BIT == 32
	bfi_raw(secret)[0] = 0xbeefbeefUL;
	bfi_raw(secret)[1] = 0xbeefbeefUL;
	bfi_raw(secret)[2] = 0xbeefbeefUL;
	bfi_raw(secret)[3] = 0xbeefbeefUL;
	#endif

	printf("Secret: \t\t");
	bfi_print(secret);

	ciphertext = bfi_mod_exp(secret, pub->exp, pub->mod);

	printf("Ciphertext: \t\t");
	bfi_print(ciphertext);

	decrypted = bfi_mod_exp(ciphertext, priv->exp, priv->mod);

	printf("Decrypted: \t\t");
	bfi_print(decrypted);

	if (!bfi_cmp(decrypted, secret))
		ret = 0;

	bfi_free(secret);
	bfi_free(ciphertext);
	bfi_free(decrypted);
	free_key(pub);
	free_key(priv);

	return ret;
}
