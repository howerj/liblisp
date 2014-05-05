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
CCOPTS=-ansi -Wall -g -Wno-write-strings -Wshadow -Wextra -pedantic -O2 -save-temps -Wswitch-enum -Wuninitialized -Wmaybe-uninitialized -Wsuggest-attribute=const

## Long strings passed to commands

RMSTR=*.o lisp *~ *.i *.s

INDENTSTR=-v -linux -nut -i2 -l120 -lc120 *.h *.c 
SPLINTSTR=-forcehint *.h *.c

DOXYFILE=doxygen.conf

## Help

all: banner lisp

banner:
	@/bin/echo -e "Howe Forth, GNU Makefile\n"
	@/bin/echo -e "Author:    $(BLUE)Richard James Howe$(DEFAULT)."
	@/bin/echo -e "Copyright: $(BLUE)Copyright 2013 Richard James Howe.$(DEFAULT)."
	@/bin/echo -e "License:   $(BLUE)LGPL$(DEFAULT)."
	@/bin/echo -e "Email:     $(BLUE)howe.r.j.89@gmail.com$(DEFAULT)."
	@/bin/echo -e "\n"

help:
	@/bin/echo "Options:"
	@/bin/echo "make"
	@/bin/echo "     Print out banner, this help message and compile program."
	@/bin/echo "make pretty"
	@/bin/echo "     Indent source, print out word count, clean up directory."
	@/bin/echo "make clean"
	@/bin/echo "     Clean up directory."
	@/bin/echo "make html"
	@/bin/echo "     Compile the documentation."

## Main program

regex.o: regex.h regex.c
	@echo "regex.o"
	@$(CC) $(CCOPTS) -c regex.c -o regex.o

sexpr.o: sexpr.h sexpr.c
	@echo "sexpr.o"
	@$(CC) $(CCOPTS) -c sexpr.c -o sexpr.o

lisp: lisp.c sexpr.o regex.o
	@echo "lisp"
	@$(CC) $(CCOPTS) lisp.c sexpr.o regex.o -o lisp

# Indent the files, clean up directory, word count.
pretty: 
	@/bin/echo -e "$(BLUE)"
	@/bin/echo -e "Indent files and clean up system.$(DEFAULT)"
	@/bin/echo -e "$(GREEN)"
	@/bin/echo "indent $(INDENTSTR)"
	@indent $(INDENTSTR);
	@/bin/echo -e "$(RED)"
	@rm -vf $(RMSTR);
	@rm -vrf doc/doxygen;
	@/bin/echo -e "$(DEFAULT)"
	@wc *.c *.h makefile

# Clean up directory.
clean:
	@/bin/echo -e "$(RED)$(RMSTR)"
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


ctags:
	@ctags -R .

doxy:
	@doxygen $(DOXYFILE)

doxygen: doxy

