#pragma once

struct rsa_key;

int rsa_cipher_test(void);
void rsa_generate_keypair(struct rsa_key **pub, struct rsa_key **priv, int bits);
void rsa_aes128_encrypt(struct rsa_key *key, void *aes_in, void **out, int *out_len);
void rsa_aes128_decrypt(struct rsa_key *key, void **aes_out, void *in, int in_len);
