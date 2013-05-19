#
# Howe Forth: Makefile
# @author         Richard James Howe.
# @copyright      Copyright 2013 Richard James Howe.
# @license        LGPL      
# @email          howe.r.j.89@gmail.com
#
#


## Colors

BLUE=\e[1;34m
GREEN=\e[1;32m
RED=\e[1;31m
DEFAULT=\e[0m

## Compiler options

CC=gcc
CCOPTS=-ansi -g -Wall -Wno-write-strings -Wshadow -Wextra -pedantic -O2
#CCOPTS=-ansi --coverage -g -Wall -Wno-write-strings -Wshadow -Wextra -pedantic -O2

## Help

all: banner lisp

banner:
	@echo -e "Howe Lsip, GNU Makefile\n"
	@echo -e "Author:    $(BLUE)Richard James Howe$(DEFAULT)."
	@echo -e "Copyright: $(BLUE)Copyright 2013 Richard James Howe.$(DEFAULT)."
	@echo -e "License:   $(BLUE)LGPL$(DEFAULT)."
	@echo -e "Email:     $(BLUE)howe.r.j.89@gmail.com$(DEFAULT)."
#	@echo -e "Add the following to \"CCOPT\" for different functionality"
#	@echo -e "To compile with debug flags enable type $(BLUE)\"-DDEBUG_PRN\".$(DEFAULT)";
#	@echo -e "To compile with debug cycle counter enabled $(BLUE)\"-DRUN4X\".$(DEFAULT)";
#	@echo -e "To compile $(RED)without$(DEFAULT) bounds checking:$(BLUE) \"-DUNCHECK\".$(DEFAULT)";
#	@echo -e "\n"


help:
	@echo "Options:"
	@echo "make"
	@echo "     Print out banner, this help message and compile program."
	@echo "make lisp"
	@echo "     Compile Howe Lisp."
	@echo "make pretty"
	@echo "     Indent source, print out word count, clean up directory."
	@echo "make clean"
	@echo "     Clean up directory."
	@echo "make gcov"
	@echo "     Compile program with coverage options, run program, run gcov."
	@echo "make splint"
	@echo "     Run splint on all *.c and *.h source files."
	@echo "make html"
	@echo "     Compile the documentation."
	@echo "make valgrind"
	@echo "     Run program with valgrind on start up lisp file only."

## Main forth program

lisp: main.c lisp.h lisp.o
	$(CC) $(CCOPTS) main.c lisp.o -o lisp

lisp.o: lisp.c lisp.h
	$(CC) $(CCOPTS) -c lisp.c -o lisp.o

## Optional extras, helper functions

# Indent the files, clean up directory, word count.
pretty: 
	@echo -e "$(BLUE)"
	@echo -e "Indent files and clean up system.$(DEFAULT)"
	@echo -e "$(GREEN)"
	@echo "indent -nut -linux *.h *.c";
	@indent -nut -linux *.h *c;
	@echo -e "$(RED)"
	@rm -vf forth memory.txt *.log *.swo *.swp *.o *~ *.gcov *.gcda *.gcno *.html *.htm;
	@echo -e "$(DEFAULT)"
	@wc *.c *.h makefile

# Clean up directory.
clean:
	@echo -e "$(RED)"
	@rm -vf forth memory.txt *.log *.swo *.swp *.o *~ *.gcov *.gcda *.gcno *.html *.htm;
	@echo -e "$(DEFAULT)"

# Static checking.
splint:
	@echo "Running \"splint *.c *.h &> splint.log\""
	-splint *.c *.h &> splint.log 

html:
	@echo -e "Compiling markdown to html."
	@for i in *.md; do echo "$$i > $$i.html"; markdown $$i > $$i.html; done

valgrind: lisp
	@echo "Running valgrind on ./lisp"
	@echo "  This command needs changing in the makefile"
	-valgrind ./lisp &> valgrind.log << EOF

gcov: CCOPTS:=$(CCOPTS) --coverage
gcov: clean lisp 
	@echo "Providing gcov statistics for forth program."
	@./lisp << EOF
	@gcov lisp.c main.c


