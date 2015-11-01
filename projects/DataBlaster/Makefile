db:
	gcc -c -O2 -Wall -fomit-frame-pointer src/main.c -o bin/main.o
	gcc -c -O2 -Wall -fomit-frame-pointer src/common.c -o bin/common.o
	gcc -c -O2 -Wall -fomit-frame-pointer src/send.c -o bin/send.o
	gcc -c -O2 -Wall -fomit-frame-pointer src/recv.c -o bin/recv.o
	gcc -O3 -fomit-frame-pointer -lm bin/main.o bin/common.o bin/send.o bin/recv.o -o db

debug:
	gcc -c -g -O0 -Wall src/main.c -o bin/main.o
	gcc -c -g -O0 -Wall src/common.c -o bin/common.o
	gcc -c -g -O0 -Wall src/send.c -o bin/send.o
	gcc -c -g -O0 -Wall -fomit-frame-pointer src/recv.c -o bin/recv.o
	gcc -g -O0 -lm bin/main.o bin/common.o bin/send.o bin/recv.o -o db

clean:
	rm db bin/*
