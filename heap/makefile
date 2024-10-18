CC=gcc
CFLAGS=-Wall -Werror -g

all: test

heap.o: heap.c heap.h
	$(CC) $(CFLAGS) -c heap.c

test: heap_test.c heap.o
	$(CC) $(CFLAGS) heap_test.c heap.o -o $@

clean:
	rm heap.o test test.dSYM/

