/* warpwallet: C implementation of the scheme from https://keybase.io/warp
 *
 * The SSE version is roughly 24000% faster than the JS implementation on the
 * website. Although that shouldn't surprise anybody. ;)
 *
 * Thrown together by Calvin Owens
 *
 * Scrypt/PBKDF2 from: http://www.tarsnap.com/scrypt/scrypt-1.1.6.tgz
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#define PRINT_HEX_BLOB(x,n) \
do { \
	printf("0x"); \
	for (int i = 0; i < n; i++) \
		printf("%02hhx", (unsigned char) x[i]); \
	printf("\n"); \
} while(0)

int crypto_scrypt(const uint8_t *passwd, size_t passwdlen, const uint8_t *salt, size_t saltlen, uint64_t N, uint32_t r, uint32_t p, uint8_t *buf, size_t buflen);
void PBKDF2_SHA256(const uint8_t *passwd, size_t passwdlen, const uint8_t *salt, size_t saltlen, uint64_t c, uint8_t *buf, size_t dkLen);

static void get_keypair_generator(char *password, int pw_len, char *salt, int sl_len, char *buf1, char *buf2, char *out)
{
	password[pw_len] = '\x01';
	salt[sl_len] = '\x01';
	crypto_scrypt((uint8_t*) password, pw_len + 1, (uint8_t*) salt, sl_len + 1, 262144, 8, 1, (uint8_t*) buf1, 32);
	password[pw_len] = '\x02';
	salt[sl_len] = '\x02';
	PBKDF2_SHA256((uint8_t*) password, pw_len + 1, (uint8_t*) salt, sl_len + 1, 65536, (uint8_t*) buf2, 32);

	((uint64_t*) out)[0] = ((uint64_t*) buf1)[0] ^ ((uint64_t*) buf2)[0];
	((uint64_t*) out)[1] = ((uint64_t*) buf1)[1] ^ ((uint64_t*) buf2)[1];
	((uint64_t*) out)[2] = ((uint64_t*) buf1)[2] ^ ((uint64_t*) buf2)[2];
	((uint64_t*) out)[3] = ((uint64_t*) buf1)[3] ^ ((uint64_t*) buf2)[3];
}

int main(int nargs, char **argv)
{
	char *s1 = calloc(4, 8);
	char *s2 = calloc(4, 8);
	char *s3 = calloc(4, 8);

	if (nargs != 3) {
		printf("Usage: %s password salt\n", argv[0]);
		return -1;
	}

	int pw_len = strlen(argv[1]);
	char *pw = calloc(pw_len + 1, 1);
	strcpy(pw, argv[1]);

	int sl_len = strlen(argv[2]);
	char *sl = calloc(sl_len + 1, 1);
	strcpy(sl, argv[2]);

	pw[pw_len] = '\xff';
	sl[sl_len] = '\xff';

	get_keypair_generator(pw, pw_len, sl, sl_len, s1, s2, s3);

	PRINT_HEX_BLOB(s3,32);

	return 0;
}
