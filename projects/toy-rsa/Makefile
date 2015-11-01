CC=gcc
CFLAGS=-O3 -c -Wall -Wextra -Werror=implicit-function-declaration -Winline -Wno-unused-function -Wno-unused-parameter -std=gnu99
LDFLAGS=-lrt
SOURCES=aes.c rng.c bfi.c rsa.c main.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=toy-rsa

all: $(SOURCES) $(EXECUTABLE)

debug: debugcflag $(GENHDRS) $(SOURCES) $(EXECUTABLE)
debug32: debug32flags debugcflag $(GENHDRS) $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o toy-rsa

debugcflag: CFLAGS += -DDEBUG -DURANDOM
debugcflag: all
debug32flags: CFLAGS += -DDEBUG -DURANDOM -m32
debug32flags: LDFLAGS += -m32
debug32flags: all
