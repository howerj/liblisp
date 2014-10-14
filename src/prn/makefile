###############################################################################
# File:     Makefile                                                          #
# Project:  regex engine test                                                 #
###############################################################################

.PHONY: all indent clean

SHELL=/bin/sh
INDENT=-linux -nut -l 150
CC=gcc
CCOPTS=-Wall -Wextra -ansi -pedantic -Os -g

all: print

print.o: print.c makefile
	$(CC) $(CCOPTS) -c print.c -o print.o

print: print.o 
	$(CC) print.o -o print

clean:
	rm -f *~ *.o print *.log

indent:
	indent $(INDENT) *.c *.h
	rm -f *~

