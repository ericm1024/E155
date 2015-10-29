CC=gcc
CFLAGS=-g -std=c11 -Wall -Wextra -pedantic
LIBPI=../lib/libpi.o

lab7: lab7.c
	$(CC) $(CFLAGS) -o $@ $^ $(LIBPI)

clean:
	rm -f lab7
