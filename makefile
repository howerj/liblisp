###############################################################################
# File:     Makefile                                                          #
# Author:   Richard James Howe                                                #
# Project:  LSP Lisp Interpreter Makefile                                     #
# License:  LGPL v2.1 or later version                                        #
###############################################################################

MAKEFLAGS+= --no-builtin-rules
.PHONY: all doxygen indent report clean valgrind help banner

## Variables ##################################################################

SHELL=/bin/sh
REPORT_DIR=doc/log
# if using musl-gcc then we can statically link our executable and it will still
# be small so add -static to CFLAGS
CC?=gcc
INPUTF=lsp/lib.lsp /dev/stdin
INDENT=-linux -nut -l 150
CFLAGS+= -Wall -Wextra -ansi -pedantic -Wswitch-enum -Os -g 
TARGET=lisp
BUILD_DIR=bin
SOURCE_DIR=src

BLUE=\e[1;34m
GREEN=\e[1;32m
RED=\e[1;31m
DEFAULT=\e[0m

OBJFILES=$(BUILD_DIR)/io.o \
	 $(BUILD_DIR)/mem.o \
	 $(BUILD_DIR)/gc.o \
	 $(BUILD_DIR)/sexpr.o \
	 $(BUILD_DIR)/lisp.o \
	 $(BUILD_DIR)/main.o

## building ###################################################################
# Only a C tool chain is necessary to built the project. Anything else is
# simply extra fluff.

all: banner $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(SOURCE_DIR)/*.h makefile
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) -o $(BUILD_DIR)/$(TARGET)

run: $(BUILD_DIR)/$(TARGET)
	$(BUILD_DIR)/./$(TARGET) -c $(INPUTF)

## testing ####################################################################

valgrind: $(BUILD_DIR)/lisp
	valgrind $(BUILD_DIR)/./$(TARGET) -c $(INPUTF)

## documentation ##############################################################

doxygen: doc/doxygen.conf
	-doxygen doc/doxygen.conf

indent:
	indent $(INDENT) $(SOURCE_DIR)/*.c $(SOURCE_DIR)/*.h

report:
	-echo "Reports generated in $(REPORT_DIR)"
	-splint -weak $(SOURCE_DIR)/*.c $(SOURCE_DIR)/*.h &> $(REPORT_DIR)/splint.log
	-wc $(SOURCE_DIR)/*.c $(SOURCE_DIR)/*.h &> $(REPORT_DIR)/wc.log
	-readelf -sW $(BUILD_DIR)/*.o &> $(REPORT_DIR)/elf.log

## help and messages ##########################################################

banner:
	@/bin/echo -e "$(GREEN)LSP, a small lisp interpreter, GNU Makefile.$(DEFAULT)"
	@/bin/echo -e "Author:    $(BLUE)Richard James Howe$(DEFAULT)."
	@/bin/echo -e "Copyright: $(BLUE)Copyright 2013 Richard James Howe.$(DEFAULT)."
	@/bin/echo -e "License:   $(BLUE)LGPL$(DEFAULT)."
	@/bin/echo -e "Email:     $(BLUE)howe.r.j.89@gmail.com$(DEFAULT)."

help:
	@/bin/echo "Options:"
	@/bin/echo "make"
	@/bin/echo "     Print out banner, this help message and compile program."
	@/bin/echo "make indent"
	@/bin/echo "     Pretty print the source."
	@/bin/echo "make clean"
	@/bin/echo "     Clean up directory."
	@/bin/echo "make report"
	@/bin/echo "     Generate reports in '$(REPORT_DIR)'."
	@/bin/echo "make doxygen"
	@/bin/echo "     Run doxygen on the source."
	@/bin/echo "make run"
	@/bin/echo "     Make and run the program."
	@/bin/echo "make valgrind"
	@/bin/echo "     Run program with valgrind on the compiled program."

## cleanup ####################################################################

clean:
	-rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/$(TARGET) doc/htm/ doc/man doc/latex $(SOURCE_DIR)/*~
	-rm -rf $(REPORT_DIR)/*.i $(REPORT_DIR)/*.s $(REPORT_DIR)/*.log
	-rm -rf CMakeFiles cmake_install.cmake CMakeCache.txt
	-rm -rf *.log

## EOF ########################################################################
