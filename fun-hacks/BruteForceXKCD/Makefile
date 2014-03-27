brute_xkcd_hash:
	gcc -I. -std=gnu99 -O2 -c main.c
	gcc -I. -std=gnu99 -O2 -c ubi1024.c
	gcc -I. -std=gnu99 -O2 -c memxor_c.c
	gcc -I. -std=gnu99 -O2 -c threefish1024_enc.c
	gcc -I. -std=gnu99 -O2 -c threefish_invmix_c.c
	gcc -I. -std=gnu99 -O2 -c threefish_mix_c.c
	gcc -O2 main.o ubi1024.o memxor_c.o threefish1024_enc.o threefish_invmix_c.o threefish_mix_c.o -o brute_xkcd_hash

static:
	gcc -I. -std=gnu99 -O2 -c main.c
	gcc -I. -std=gnu99 -O2 -c ubi1024.c
	gcc -I. -std=gnu99 -O2 -c memxor_c.c
	gcc -I. -std=gnu99 -O2 -c threefish1024_enc.c
	gcc -I. -std=gnu99 -O2 -c threefish_invmix_c.c
	gcc -I. -std=gnu99 -O2 -c threefish_mix_c.c
	gcc -static -O2 main.o ubi1024.o memxor_c.o threefish1024_enc.o threefish_invmix_c.o threefish_mix_c.o -o brute_xkcd_hash

clean:
	rm -rf *.o
	rm -rf brute_xkcd_hash
