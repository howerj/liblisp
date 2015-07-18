CC=gcc
CCFLAGS=-Wall -Wextra 

all: tb_hash

tb_hash: tb_hash.c hash.o hash.h
	$(CC) $(CCFLAGS) hash.o tb_hash.c -o tb_hash

hash.o: hash.c hash.h
	$(CC) $(CCFLAGS) hash.c -c -o hash.o

indent:
	indent -linux -l 150 -nut *.c *.h
	rm *~

clean:
	rm -f tb_hash *.o *~
