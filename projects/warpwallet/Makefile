gen:
	gcc -O3 -std=gnu99 -Wall -Wextra crypto_scrypt-ref.c sha256.c main.c -lssl -lrt -o warpwallet
sse:
	gcc -O3 -std=gnu99 -Wall -Wextra crypto_scrypt-sse.c sha256.c main.c -lssl -lrt -o warpwallet
test:
	gcc -O3 -std=gnu99 -Wall -Wextra crypto_scrypt-ref.c sha256.c test-main.c -lssl -lrt -o warpwallet
testsse:
	gcc -O3 -std=gnu99 -Wall -Wextra crypto_scrypt-sse.c sha256.c test-main.c -lssl -lrt -o warpwallet
clean:
	rm -rf warpwallet
