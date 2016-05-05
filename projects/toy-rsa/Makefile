CC = gcc
CFLAGS = -O3 -c -fno-strict-aliasing -D_GNU_SOURCE -Wall -Wextra \
	-Werror=implicit-function-declaration -Wdeclaration-after-statement \
	-Wstrict-prototypes -Wmissing-prototypes -Wmissing-declarations \
	-Winline -Wno-unused-function -Wno-unused-parameter

prof: CFLAGS += -DCONSTRNG -DTESTCOUNT=1

debug: CFLAGS += -O1 -gdwarf-4 -fno-omit-frame-pointer -fstack-protector-all \
	-fsanitize=address -fsanitize=undefined
debug: LDFLAGS += -lasan -lubsan

obj = bfi.o aes.o rng.o rsa.o main.o
binary = toy-rsa

all: $(obj) $(binary)
prof: all
debug: all

$(binary): $(obj)
	$(CC) $(LDFLAGS) $(obj) -o $@

%.o: %.c
	$(CC) $< $(CFLAGS) $(INCLUDES) -c -o $@

clean:
	rm -f *.o toy-rsa
