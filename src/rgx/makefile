###############################################################################
# File:     Makefile                                                          #
# Project:  regex engine test                                                 #
###############################################################################

.PHONY: all indent clean

SHELL=/bin/sh
INDENT=-linux -nut -l 150
CC=gcc
CCOPTS=-Wall -Wextra -ansi -pedantic -Os -g

all: grep

regex.o: regex.c regex.h makefile
	$(CC) $(CCOPTS) -c regex.c -o regex.o

grep: grep.c regex.o
	$(CC) grep.c regex.o -o grep

clean:
	rm -f *~ *.o grep *.log

indent:
	indent $(INDENT) *.c *.h
	rm -f *~

