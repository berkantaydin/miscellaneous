#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>

#define PRINT_HEX_BLOB(x,n) \
do { \
	printf("0x"); \
	for (int i = 0; i < n; i++) \
		printf("%02hhx", (unsigned char) x[i]); \
	printf("\n"); \
} while(0)

int crypto_scrypt(const uint8_t *passwd, size_t passwdlen, const uint8_t *salt, size_t saltlen, uint64_t N, uint32_t r, uint32_t p, uint8_t *buf, size_t buflen);
void PBKDF2_SHA256(const uint8_t *passwd, size_t passwdlen, const uint8_t *salt, size_t saltlen, uint64_t c, uint8_t *buf, size_t dkLen);

int main(void)
{
	char *buf1 = calloc(64, 8);
	char *buf2 = calloc(64, 8);

	/* Test vectors: www.tarsnap.com/scrypt/scrypt.pdf */
	crypto_scrypt((uint8_t*) "pleaseletmein", 13, (uint8_t*) "SodiumChloride", 14, 16384, 8, 1, (uint8_t*) buf1, 64);
	PRINT_HEX_BLOB(buf1, 64);

	/* Test vectors: http://stackoverflow.com/questions/5130513/pbkdf2-hmac-sha2-test-vectors */
	PBKDF2_SHA256((uint8_t*) "password", 8, (uint8_t*) "salt", 4, 4096, (uint8_t*) buf2, 32);
	PRINT_HEX_BLOB(buf2, 32);

	return 0;
}
