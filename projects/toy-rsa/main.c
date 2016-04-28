#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#include "include/aes.h"
#include "include/rsa.h"
#include "include/rng.h"

int main(int argc, char **argv)
{
	int bits = 256;

	puts("Testing RSA forever...");
	setvbuf(stdout, NULL, _IONBF, 0);

	if (argc > 1)
		bits = atoi(argv[1]);

	init_rng();

	while (!rsa_cipher_test(bits))
		puts("PASSED!");

	puts("FAILED!");
	abort();
}
