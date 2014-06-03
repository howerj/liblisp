###############################################################################
# File:			Makefile                                                          #
# Author: 	Richard James Howe                                                #
# Project: 	LSP Lisp Interpreter Makefile                                     #
# License:	GPL v2.0 or later version                                         #
###############################################################################

## Variables ##################################################################
CC=gcc 
CCFLAGS=-Wall -Wextra -ansi -pedantic -O2 -g 
OBJFILES=io.o mem.o sexpr.o lisp.o 
TSTBINARIES=repl

## building ###################################################################

all: repl

%.o: %.c *.h makefile
	$(CC) -c $(CCFLAGS) -o $@ $<

repl: $(OBJFILES) repl.o dat/repl.in dat/repl.out
	$(CC) $(CCFLAGS) $(OBJFILES) repl.o -o repl
	cat dat/repl.in | ./repl > repl.tmp
	diff dat/repl.out repl.tmp
	@echo "repl: pass";

clean:
	-rm -rf *.o $(TSTBINARIES) *.tmp

## EOF ########################################################################
