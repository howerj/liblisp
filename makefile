#
# Howe Lisp: Makefile
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
CCOPTS=-ansi -Wall -g -Wno-write-strings -Wshadow -Wextra -pedantic -O2 -save-temps  


## Long strings passed to commands

RMSTR=*.o *.log *.swo *.swp *.o *.gcov *.gcda *.gcno *.i *.s tags

INDENTSTR=-v -linux -nut -i2 -l120 -lc120 *.h *.c 
SPLINTSTR=-forcehint *.h *.c

DOXYFILE=doc/doxygen.conf

## Help

all: banner lisp

banner:
	@/bin/echo -e "Howe Lisp, GNU Makefile\n"
	@/bin/echo -e "Author:    $(BLUE)Richard James Howe$(DEFAULT)."
	@/bin/echo -e "Copyright: $(BLUE)Copyright 2013 Richard James Howe.$(DEFAULT)."
	@/bin/echo -e "License:   $(BLUE)LGPL$(DEFAULT)."
	@/bin/echo -e "Email:     $(BLUE)howe.r.j.89@gmail.com$(DEFAULT)."

help:
	@/bin/echo "Options:"
	@/bin/echo "make"
	@/bin/echo "     Print out banner, this help message and compile program."
	@/bin/echo "make lisp"
	@/bin/echo "     Compile Howe Lisp."
	@/bin/echo "make run"
	@/bin/echo "     Run Howe Lisp."
	@/bin/echo "make pretty"
	@/bin/echo "     Indent source, print out word count, clean up directory."
	@/bin/echo "make clean"
	@/bin/echo "     Clean up directory."
	@/bin/echo "make gcov"
	@/bin/echo "     Compile program with coverage options, run program, run gcov."
	@/bin/echo "make splint"
	@/bin/echo "     Run splint on all *.c and *.h source files."
	@/bin/echo "make html"
	@/bin/echo "     Compile the documentation."
	@/bin/echo "make valgrind"
	@/bin/echo "     Run program with valgrind on start up Lisp file only."

## Main program

# Top level
lisp: lisp.c
	$(CC) $(CCOPTS) lisp.c -o lisp
	@mv *.i *.s log/


## Optional extras, helper functions

# Run the interpreter

run: lisp
	@./lisp

# Indent the files, clean up directory, word count.
pretty: 
	@/bin/echo -e "$(BLUE)"
	@/bin/echo -e "Indent files and clean up system.$(DEFAULT)"
	@/bin/echo -e "$(GREEN)"
	@/bin/echo "indent $(INDENTSTR)"
	@indent $(INDENTSTR);
	@/bin/echo -e "$(RED)"
	@rm -vf $(RMSTR);
	@/bin/echo -e "$(DEFAULT)"
	@wc lib/*.c lib/*.h *.c fth/*.4th makefile

# Clean up directory.
clean:
	@/bin/echo -e "$(RED)"
	@rm -vf $(RMSTR);
	@rm -vrf doc/doxygen;
	@/bin/echo -e "$(DEFAULT)"

# Static checking.
splint:
	@/bin/echo "$(SPLINTSTR)";
	-splint $(SPLINTSTR);

html:
	@/bin/echo -e "Compiling markdown to html."
	for i in doc/*.md; do\
		/bin/echo "$$i > $$i.html";\
		markdown $$i > $$i.html;\
	done

valgrind: lisp
	@/bin/echo "Running valgrind on ./lisp"
	@/bin/echo "  This command needs changing in the makefile"
	-cd bin/; valgrind ./lisp << EOF

ctags:
	@ctags -R .

doxy:
	@doxygen $(DOXYFILE)

doxygen: doxy

gcov: CCOPTS:=$(CCOPTS) --coverage
gcov: clean bin/lisp
	@/bin/echo "not implemented yet"
