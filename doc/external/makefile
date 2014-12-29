CC=gcc

.PHONY: all clean

all: sl3 sl5 utf8.o

sl3: sl3.c
	$(CC) sl3.c -o sl3

sl5: sl5.c
	$(CC) sl5.c -o sl5

utf8.o: utf8.c utf8.h
	$(CC) utf8.c -c -o utf8.o

clean:
	rm -f sl3 sl5 *.o

