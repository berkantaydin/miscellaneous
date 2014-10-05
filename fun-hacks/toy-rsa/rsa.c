/* rsa.c: From-scratch implementation of RSA
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
 * This implementation does not attempt to be compliant with any standard, and
 * is horribly insecure. For the love of god, please don't actually use it.
 */

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <errno.h>

#include "common.h"
#include "aes.h"
#include "rng.h"
#include "bfi.h"

struct rsa_key {
	struct bfi *exp;
	struct bfi *mod;
};

static void free_key(struct rsa_key *r)
{
	free_bfi(r->exp);
	free_bfi(r->mod);
	free(r);
}

/* Return the modular multiplicative inverse of e mod tot */
static struct bfi *mod_inv(struct bfi *e, struct bfi *tot)
{
	struct bfi *a = new_bfi_copyof(e);
	struct bfi *b = new_bfi_copyof(tot);
	struct bfi *m = new_bfi(tot->wlen << ULONG_BITSHIFT);
	struct bfi *x_last = new_bfi(tot->wlen << ULONG_BITSHIFT);
	struct bfi *x = new_bfi(tot->wlen << ULONG_BITSHIFT);
	struct bfi *q = NULL;
	struct bfi *r = NULL;
	struct bfi *tmp = NULL;

	x->n[0] = 1;
	while (!bfi_is_zero(a)) {
		do_bfi_div(b,a,&q,&r);

		xchg_bfi(m,x_last);
		do_bfi_multiply(&tmp,q,x);
		do_bfi_sub(m,tmp);
		free_bfi(tmp);

		xchg_bfi(x_last,x);
		xchg_bfi(x,m);
		xchg_bfi(b,a);
		xchg_bfi(a,r);

		free_bfi(r);
		free_bfi(q);
	}

	if (x_last->sign)
		do_bfi_add(x_last,tot);
	do_bfi_mod(x_last,tot);

	return x_last;
}

static int bfi_is_prime(struct bfi *n)
{
	struct bfi *rnd = new_bfi(n->wlen << ULONG_BITSHIFT);
	struct bfi *res = new_bfi(n->wlen << ULONG_BITSHIFT);
	struct bfi *nminusone = new_bfi_copyof(n);
	int ret = 0;

	do_bfi_dec(nminusone);
	for (int i = 0; i < 10; i++) {
		debug("+");
		rng_fill_mem(rnd->n, rnd->wlen << ULONG_BYTESHIFT);
		res = do_bfi_mod_exp(rnd,nminusone,n);
		if (!bfi_is_one(res))
			goto not_prime;
	}
	ret = 1; /* Probably prime */
not_prime:
	free_bfi(rnd);
	free_bfi(res);
	free_bfi(nminusone);
	try_to_shrink_bfi(n);
	return ret;
}

static struct bfi *find_prime(unsigned int bits, struct bfi *e)
{
	struct bfi *prime = new_bfi(bits);
	struct bfi *tmp;

	debug("Searching for prime");

	while (1) {
		debug(".");
		rng_fill_mem(prime->n, bits >> 3);
		prime->n[0] |= (unsigned long) 0x01; /* Don't try even numbers */

		if (bfi_is_divby_three(prime))
			continue;

		do_bfi_dec(prime);
		tmp = bfi_gcd(prime,e);

		if (!bfi_is_one(tmp)) {
			debug("!");
			free_bfi(tmp);
			continue;
		}

		free_bfi(tmp);
		do_bfi_inc(prime);

		if (bfi_is_prime(prime))
			break;
	}
	debug(" done!\n");
	return prime;
}

void rsa_generate_keypair(struct rsa_key **pub, struct rsa_key **priv, int bits)
{
	struct bfi *p, *q, *d, *mod, *tot, *secret, *ciphertext, *decrypted, *tmp;
	struct bfi *e = new_bfi(64);

	e->n[0] = 65537;

	debug("Generating RSA key...\n");
	p = find_prime(bits >> 1, e);
	q = find_prime(bits >> 1, e);
	do_bfi_multiply(&mod,p,q);
	do_bfi_dec(p);
	do_bfi_dec(q);
	do_bfi_multiply(&tot,p,q);

	d = mod_inv(e,tot);

	debug("Found a key! Testing it...\n");

	secret = new_bfi(128);
	#if ULONG_BITS == 64
	secret->n[0] = 0xbeefbeefbeefbeefUL;
	secret->n[1] = 0xbeefbeefbeefbeefUL;
	#elif ULONG_BITS == 32
	secret->n[0] = 0xbeefbeefUL;
	secret->n[1] = 0xbeefbeefUL;
	secret->n[2] = 0xbeefbeefUL;
	secret->n[3] = 0xbeefbeefUL;
	#endif

	ciphertext = do_bfi_mod_exp(secret,e,mod);
	decrypted = do_bfi_mod_exp(ciphertext,d,mod);
	if (!bfi_eq(decrypted,secret))
		fatal("Key generation failed!\n");

	free_bfi(decrypted);
	free_bfi(ciphertext);
	free_bfi(secret);

	free_bfi(tot);
	free_bfi(p);
	free_bfi(q);

	*pub = ecalloc(1, sizeof(**pub));
	*priv = ecalloc(1, sizeof(**priv));

	tmp = new_bfi_copyof(mod);

	(*pub)->exp = e;
	(*pub)->mod = mod;
	(*priv)->mod = tmp;
	(*priv)->exp = d;
}

int rsa_cipher_test(void)
{
	struct rsa_key *pub,*priv;
	struct bfi *secret, *ciphertext, *decrypted;
	int ret = -1;

	rsa_generate_keypair(&pub, &priv, 2048);

	secret = new_bfi(128);
	#if ULONG_BITS == 64
	secret->n[0] = 0xbeefbeefbeefbeefUL;
	secret->n[1] = 0xbeefbeefbeefbeefUL;
	#elif ULONG_BITS == 32
	secret->n[0] = 0xbeefbeefUL;
	secret->n[1] = 0xbeefbeefUL;
	secret->n[2] = 0xbeefbeefUL;
	secret->n[3] = 0xbeefbeefUL;
	#endif

	ciphertext = do_bfi_mod_exp(secret,pub->exp,pub->mod);
	decrypted = do_bfi_mod_exp(ciphertext,priv->exp,priv->mod);

	if (bfi_eq(decrypted,secret))
		ret = 0;

	#ifdef DEBUG
		printf("e: \t\t");
		print_bfi(pub->exp);
		printf("Modulus: \t\t");
		print_bfi(pub->mod);
		printf("d: \t\t");
		print_bfi(priv->exp);
		printf("Secret: \t\t");
		print_bfi(secret);
		printf("Ciphertext: \t\t");
		print_bfi(ciphertext);
		printf("Decrypted: \t\t");
		print_bfi(decrypted);
	#endif /* DEBUG */

	free_bfi(secret);
	free_bfi(ciphertext);
	free_bfi(decrypted);
	free_key(pub);
	free_key(priv);

	return ret;
}
