all:
	gcc -O3 -g -Wall -Wextra -fno-strict-aliasing -D_GNU_SOURCE -march=native memcpy-test.c -o test-memcpy
	gcc -O3 -g -Wall -Wextra -fno-strict-aliasing -D_GNU_SOURCE -march=native zero-test.c -o test-zero
clean:
	rm -f test-memcpy test-zero
