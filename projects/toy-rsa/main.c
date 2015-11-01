#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "aes.h"
#include "rsa.h"
#include "rng.h"

void print_arr(char *a, int len)
{
	for (int i = 0; i < len; i++)
		printf("%hhx",a[i]);
	printf("\n");
}

void test_rsa_forever(void)
{
	int i = 100;
	while (i--) {
		printf("Testing RSA... ");
		if (rsa_cipher_test()) {
			printf("FAILED!\n");
			abort();
		} else {
			printf("PASSED!\n");
		}
	}
}

int main(void)
{
	init_rng();
	printf("Hello world!\n");
	setvbuf(stdout, NULL, _IONBF, 0);
	test_rsa_forever();
}
