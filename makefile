###############################################################################
# File:			Makefile                                                          #
# Author: 	Richard James Howe                                                #
# Project: 	LSP Lisp Interpreter Makefile                                     #
# License:	GPLv3.0                                                           #
###############################################################################

## Variables ##################################################################
CC=gcc
# add -g and -pg for profiling
CCFLAGS=-Wall -Wextra -ansi -pedantic -O2 -g
OBJFILES=bin/io.o bin/mem.o bin/sexpr.o bin/lisp.o bin/main.o 

## building ###################################################################
# Only a C tool chain is necessary to built the project. Anything else is
# simply extra fluff.

all: bin/lisp

bin/%.o: src/%.c src/*.h makefile
	$(CC) -c $(CCFLAGS) -o $@ $<

bin/lisp: $(OBJFILES)
	$(CC) $(CCFLAGS) $(OBJFILES) -o bin/lisp

run: bin/lisp
	bin/./lisp lsp/lib.lsp

## testing ####################################################################

valgrind: bin/lisp
	valgrind bin/./lisp -G lsp/lib.lsp

strace: bin/lisp
	strace bin/./lisp -G lsp/lib.lsp

# Most current version of git; The idea would be to use this to generate
# a header file as a version number for the program.
# git log | head -n 1 | awk '{print $2}'

## documentation ##############################################################

doxygen:
	-doxygen doc/doxygen.conf

report:
	-splint src/*.c src/*.h
	-wc src/*.c src/*.h
	-readelf -sW bin/*.o

## cleanup ####################################################################

clean:
	-rm -rf bin/*.o bin/lisp doc/htm/ doc/man doc/latex

## EOF ########################################################################
