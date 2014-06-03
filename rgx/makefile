CC=gcc
CCOPTS=-Wall -Wextra -ansi -pedantic -O2

all: grep

regex.o: regex.c regex.h
	$(CC) $(CCOPTS) -c regex.c -o regex.o

grep: main.c regex.o
	$(CC) main.c regex.o -o grep

clean:
	rm -f *~ *.o grep *.log

indent:
	indent -linux -nut -i2 *.c *.h
	make clean

