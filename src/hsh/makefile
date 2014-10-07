CC=gcc
CCFLAGS=-Wall -Wextra 

all: hash

hash: test.c hash.o hash.h
	$(CC) $(CCFLAGS) hash.o test.c -o hash

hash.o: hash.c hash.h
	$(CC) $(CCFLAGS) hash.c -c -o hash.o

indent:
	indent -linux -l 150 -nut *.c *.h
	rm *~

clean:
	rm -f hash *.o *~
