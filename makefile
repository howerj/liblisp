# liblisp
# A lisp library and interpreter
# Released under the LGPL license
CC      ?= gcc
CFLAGS 	?= -Wall -Wextra -fwrapv -std=c99 -pedantic -Os
TARGET   = lisp
.PHONY: all clean doc run 

all: $(TARGET)

lib$(TARGET).a: lib$(TARGET).o
	ar rcs $@ $<

lib$(TARGET).o: lib$(TARGET).c lib$(TARGET).h 
	$(CC) $(CFLAGS) $< -c -o $@

main.o: main.c lib$(TARGET).h 
	$(CC) $(CFLAGS) $< -c -o $@

$(TARGET): main.o lib$(TARGET).a
	$(CC) $(CFLAGS) $^ -o $@

lib$(TARGET).htm: lib$(TARGET).md
	markdown $^ > $@

run: $(TARGET)
	@echo running the executable with default arguments
	./$^ -pc init.lsp test.lsp -

clean:
	@echo Cleaning repository.
	rm -rf $(TARGET) *.a *.so *.o *.bak *.htm *.html 
	rm -rf *.log *.out *.bak *.tgz *~

##############################################################################
