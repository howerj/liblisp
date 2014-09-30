CC=gcc
CFLAGS=-Wall -Wextra -ansi -pedantic -g

all: bignum

.PHONY: clean all indent splint

%.o: %.c *.h makefile
	$(CC) -c $(CFLAGS) -o $@ $<

bignum: bignum.o check.o main.c
	$(CC) $(CFLAGS) bignum.o check.o main.c -o bignum

indent:
	indent -linux -nut -l 150 *.c *.h
	rm -f *~

splint:
	-splint -weak *.c *.h &> splint.log

clean:
	rm -f *.o bignum

