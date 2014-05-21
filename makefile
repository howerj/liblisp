###############################################################################
# File:			Makefile                                                          #
# Author: 	Richard James Howe                                                #
# Project: 	LSP Lisp Interpreter Makefile                                     #
# License:	GPL v2.0 or later version                                         #
###############################################################################

## Variables ##################################################################
REPORT_DIR=doc/log
CC=gcc # clang should work as well.
# add -g and -pg for profiling
CCFLAGS=-Wall -Wextra -ansi -pedantic -O2 -g -save-temps -fverbose-asm
OBJFILES=bin/io.o bin/mem.o bin/sexpr.o bin/lisp.o bin/main.o 

## building ###################################################################
# Only a C tool chain is necessary to built the project. Anything else is
# simply extra fluff.

all: bin/lisp

bin/%.o: src/%.c src/*.h makefile
	$(CC) -c $(CCFLAGS) -o $@ $<

bin/lisp: $(OBJFILES)
	$(CC) $(CCFLAGS) $(OBJFILES) -o bin/lisp
	-mv *.i *.s $(REPORT_DIR)

run: bin/lisp
	bin/./lisp lsp/lib.lsp

## testing ####################################################################

valgrind: bin/lisp
	valgrind bin/./lisp -G lsp/lib.lsp

strace: bin/lisp
	strace bin/./lisp -G lsp/lib.lsp

ltrace: bin/lisp
	ltrace bin/./lisp -G lsp/lib.lsp

# Most current version of git; The idea would be to use this to generate
# a header file as a version number for the program.
# git log | grep "^commit" | head -n 1 | awk '{print $2}'

## documentation ##############################################################

doxygen:
	-doxygen doc/doxygen.conf


report:
	-echo "Reports generated in $(REPORT_DIR)"
	-splint -weak src/*.c src/*.h &> $(REPORT_DIR)/splint.log
	-wc src/*.c src/*.h &> $(REPORT_DIR)/wc.log
	-readelf -sW bin/*.o &> $(REPORT_DIR)/elf.log

tar:
	make clean
	tar cf /tmp/backup.tar .

## cleanup ####################################################################

clean:
	-rm -rf bin/*.o bin/lisp doc/htm/ doc/man doc/latex
	-rm -rf $(REPORT_DIR)/*.i $(REPORT_DIR)/*.s $(REPORT_DIR)/*.log

## EOF ########################################################################
