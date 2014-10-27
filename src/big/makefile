CC=gcc
CFLAGS=-Wall -Wextra -ansi -pedantic -g -O3

all: tb_bignum

.PHONY: clean all indent splint

%.o: %.c *.h makefile
	$(CC) -c $(CFLAGS) -o $@ $<

tb_bignum: bignum.o tb_bignum.c
	$(CC) $(CFLAGS) bignum.o tb_bignum.c -o tb_bignum

indent:
	indent -linux -nut -l 150 *.c *.h
	rm -f *~

splint:
	-splint -weak *.c *.h &> splint.log

clean:
	rm -f *.o tb_bignum

