# Boilerplate makefile, because lazy

CC=gcc
CFLAGS=-O3 -DDEBUG -pipe -c -Wall -Wextra -Werror=implicit-function-declaration -Winline -Wno-unused-parameter -std=gnu99
LDFLAGS=-lrt
SOURCES=main.c board.c negamax.c
OBJECTS=$(SOURCES:.c=.o)
EXECUTABLE=chess-engine

all: $(SOURCES) $(EXECUTABLE)

$(EXECUTABLE): $(ASMOBJ) $(OBJECTS)
	$(CC) $(LDFLAGS) $(OBJECTS) -o $@

.o:
	$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f *.o chess-engine gmon.out
