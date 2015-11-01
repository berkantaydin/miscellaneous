#pragma once
#include <stdint.h>

struct aes_ctx {
	uint32_t key_enc[60];
	uint32_t key_dec[60];
};

void aes128_expand_key(struct aes_ctx *ctx, void *in_key);
void aes128_encrypt(struct aes_ctx *ctx, void *buf);
void aes128_decrypt(struct aes_ctx *ctx, void *buf);
int aes_cipher_test(void);
