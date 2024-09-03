CC=gcc
CFLAGS=-Wall -Werror -g

all: test

hmap.o: hmap.c hmap.h
	$(CC) $(CFLAGS) -c hmap.c

test: hmap_test.c hmap.o
	$(CC) $(CFLAGS) hmap_test.c hmap.o -o $@

clean:
	rm hmap.o test

