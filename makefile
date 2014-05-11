CC=gcc
CCFLAGS=-Wall -Wextra -ansi -pedantic -O2
OBJFILES=bin/io.o bin/mem.o bin/sexpr.o bin/lisp.o bin/main.o 

all: bin/lisp

bin/%.o: src/%.c src/*.h 
	$(CC) -c $(CCFLAGS) -o $@ $<

bin/lisp: $(OBJFILES)
	$(CC) $(CCFLAGS) $(OBJFILES) -o bin/lisp

run: bin/lisp
	bin/./lisp

test: bin/lisp
	cat lsp/tst.lsp - | bin/./lisp -G

valgrind: bin/lisp
	cat lsp/tst.lsp - | valgrind bin/./lisp -G

strace: bin/lisp
	cat lsp/tst.lsp - | strace bin/./lisp -G


doxygen:
	-doxygen doc/doxygen.conf

report:
	-splint src/*.c src/*.h
	-wc src/*.c src/*.h

clean:
	-rm -rf bin/*.o bin/lisp doc/htm/ doc/man doc/latex

