###############################################################################
# File:       Makefile                                                        #
# Author:     Richard James Howe                                              #
# Project:    LSP Lisp Interpreter Makefile                                   #
# License:    LGPL v2.0 or later version                                      #
###############################################################################

## Variables ##################################################################
CC=gcc 
CPPC=g++
CFLAGS=-Wall -Wextra -ansi -pedantic -O2 -g -I$(INCLUDE_DIR)
BIN_DIR=../bin
INCLUDE_DIR=../src
CPPFLAGS=-Wall -Wextra -O2 -g -I$(INCLUDE_DIR)

IO_O=$(BIN_DIR)/io.o
MEM_O=$(BIN_DIR)/mem.o
GC_O=$(BIN_DIR)/gc.o
SEXPR_O=$(BIN_DIR)/sexpr.o
LISP_O=$(BIN_DIR)/lisp.o
REGEX_O=$(BIN_DIR)/regex.o

OBJFILES=$(IO_O) $(MEM_O) $(GC_O) $(SEXPR_O) $(LISP_O) $(REGEX_O)

TEST_BINARIES=tb_repl tb_repl.cpp.exe tb_sexpr

## building ###################################################################

.PHONY: all clean

all: $(TEST_BINARIES)

tb_repl: $(OBJFILES) tb_repl.o dat/repl.in dat/repl.out
	$(CC) $(CFLAGS) $(OBJFILES) tb_repl.o -o tb_repl
	cat  dat/repl.in | ./tb_repl > repl.tmp
	diff dat/repl.out repl.tmp
	@echo "tb_repl: pass";

tb_repl.cpp.exe: $(OBJFILES) tb_repl.cpp.o
	$(CPPC) $(CPPFLAGS) $(OBJFILES) tb_repl.cpp.o -o tb_repl.cpp.exe

tb_repl.cpp.o: tb_repl.cpp makefile
	$(CPPC) $(CPPFLAGS) tb_repl.cpp -c -o tb_repl.cpp.o

tb_sexpr: $(IO_O) $(MEM_O) $(GC_O) $(SEXPR_O) tb_sexpr.o dat/sexpr.in dat/sexpr.out
	$(CC) $(CFLAGS) $(IO_O) $(MEM_O) $(GC_O) $(SEXPR_O) tb_sexpr.o -o tb_sexpr
	cat  dat/sexpr.in | ./tb_sexpr > sexpr.tmp
	diff dat/sexpr.out sexpr.tmp
	@echo "tb_sexpr: pass";

clean:
	-rm -rf *.o *.tmp $(TEST_BINARIES) *~

## EOF ########################################################################
