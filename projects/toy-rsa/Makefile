CC = gcc
CFLAGS = -O3 -c -fno-strict-aliasing -D_GNU_SOURCE -Wall -Wextra \
	-Werror=implicit-function-declaration -Wdeclaration-after-statement \
	-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
	-Winline -Wno-unused-function -Wno-unused-parameter

#CFLAGS += -gdwarf-4 -fsanitize=address -fsanitize=undefined
#LDFLAGS = -lasan -lubsan

obj = bfi.o aes.o rng.o rsa.o main.o
binary = toy-rsa

all: $(obj) $(binary)

$(binary): $(obj)
	$(CC) $(LDFLAGS) $(obj) -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDES) -c -o $@

clean:
	rm -f *.o toy-rsa
