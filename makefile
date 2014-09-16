###############################################################################
# File:     Makefile                                                          #
# Author:   Richard James Howe                                                #
# Project:  LSP Lisp Interpreter Makefile                                     #
# License:  LGPL v2.1 or later version                                        #
###############################################################################

.PHONY: all doxygen indent report tar clean valgrind strace ltrace 

## Variables ##################################################################

SHELL=/bin/sh
REPORT_DIR=doc/log
CC=gcc
INPUTF=lsp/lib.lsp /dev/stdin
INDENT=-linux -nut -l 150
CFLAGS=-Wall -Wextra -ansi -pedantic -Os -g
OBJFILES=bin/io.o bin/mem.o bin/gc.o bin/sexpr.o bin/lisp.o bin/main.o

## building ###################################################################
# Only a C tool chain is necessary to built the project. Anything else is
# simply extra fluff.

all: bin/lisp

bin/%.o: src/%.c src/*.h makefile
	$(CC) -c $(CFLAGS) -o $@ $<

bin/lisp: $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) -o bin/lisp

run: bin/lisp
	bin/./lisp -c $(INPUTF)

## testing ####################################################################

valgrind: bin/lisp
	valgrind bin/./lisp -cG $(INPUTF)

ltrace: bin/lisp
	ltrace bin/./lisp -cG $(INPUTF)

## documentation ##############################################################

doxygen: doc/doxygen.conf
	-doxygen doc/doxygen.conf

indent:
	indent $(INDENT) src/*.c src/*.h

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
	-rm -rf bin/*.o bin/lisp doc/htm/ doc/man doc/latex src/*~
	-rm -rf $(REPORT_DIR)/*.i $(REPORT_DIR)/*.s $(REPORT_DIR)/*.log
	-rm -rf *.log

## EOF ########################################################################
