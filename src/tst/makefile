###############################################################################
# File:       Makefile                                                        #
# Author:     Richard James Howe                                              #
# Project:    LSP Lisp Interpreter Makefile                                   #
# License:    LGPL v2.0 or later version                                      #
###############################################################################

## Variables ##################################################################
CC=gcc 
CCFLAGS=-Wall -Wextra -ansi -pedantic -O2 -g 
OBJFILES=io.o mem.o gc.o sexpr.o lisp.o
TSTBINARIES=repl sexpr

## building ###################################################################

all: sexpr repl 

%.o: %.c *.h makefile
	$(CC) -c $(CCFLAGS) -o $@ $<

repl: $(OBJFILES) repl.o dat/repl.in dat/repl.out
	$(CC) $(CCFLAGS) $(OBJFILES) repl.o -o repl
	cat  dat/repl.in | ./repl > repl.tmp
	diff dat/repl.out repl.tmp
	@echo "repl: pass";

sexpr: io.o mem.o gc.o sexpr.o tb_sexpr.o dat/sexpr.in dat/sexpr.out
	$(CC) $(CCFLAGS) io.o mem.o sexpr.o gc.o tb_sexpr.o -o sexpr
	cat  dat/sexpr.in | ./sexpr > sexpr.tmp
	diff dat/sexpr.out sexpr.tmp
	@echo "sexpr: pass";

clean:
	-rm -rf *.o $(TSTBINARIES) *.tmp

## EOF ########################################################################
