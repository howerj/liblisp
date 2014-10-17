###############################################################################
# File:     Makefile                                                          #
# Author:   Richard James Howe                                                #
# Project:  LSP Lisp Interpreter Makefile                                     #
# License:  LGPL v2.1 or later version                                        #
###############################################################################

MAKEFLAGS+= --no-builtin-rules
.PHONY: all doxygen indent report tar clean valgrind 

## Variables ##################################################################

SHELL=/bin/sh
REPORT_DIR=doc/log
# if using musl-gcc then we can statically link our executable and it will still
# be small so add -static to CFLAGS
CC=gcc
INPUTF=lsp/lib.lsp /dev/stdin
INDENT=-linux -nut -l 150
CFLAGS+= -Wall -Wextra -ansi -pedantic -Wswitch-enum -Os -g 
TARGET=lisp
BUILD_DIR=bin
SOURCE_DIR=src

OBJFILES=$(BUILD_DIR)/io.o \
	 $(BUILD_DIR)/mem.o \
	 $(BUILD_DIR)/gc.o \
	 $(BUILD_DIR)/sexpr.o \
	 $(BUILD_DIR)/lisp.o \
	 $(BUILD_DIR)/main.o

## building ###################################################################
# Only a C tool chain is necessary to built the project. Anything else is
# simply extra fluff.

all: $(BUILD_DIR)/$(TARGET)

$(BUILD_DIR)/%.o: $(SOURCE_DIR)/%.c $(SOURCE_DIR)/*.h makefile
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) -o $(BUILD_DIR)/$(TARGET)

run: $(BUILD_DIR)/$(TARGET)
	$(BUILD_DIR)/./$(TARGET) -c $(INPUTF)

## testing ####################################################################

valgrind: $(BUILD_DIR)/lisp
	valgrind $(BUILD_DIR)/./$(TARGET) -cG $(INPUTF)

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

tar:
	make clean
	tar cf /tmp/backup.tar .

## cleanup ####################################################################

clean:
	-rm -rf $(BUILD_DIR)/*.o $(BUILD_DIR)/$(TARGET) doc/htm/ doc/man doc/latex $(SOURCE_DIR)/*~
	-rm -rf $(REPORT_DIR)/*.i $(REPORT_DIR)/*.s $(REPORT_DIR)/*.log
	-rm -rf *.log

## EOF ########################################################################
