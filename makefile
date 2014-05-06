CC=gcc
CCFLAGS=-Wall -Wextra -ansi -pedantic
OBJFILES=bin/lisp.o bin/mem.o bin/io.o bin/sexpr.o

all: bin/lisp

bin/%.o: src/%.c src/*.h 
	$(CC) -c $(CCFLAGS) -o $@ $<

bin/lisp: $(OBJFILES)
	$(CC) $(CCFLAGS) $(OBJFILES) -o bin/lisp

run: bin/lisp
	bin/./lisp

report:
	@splint src/*.c src/*.h
	@wc src/*.c src/*.h

clean:
	rm -rf bin/*.o bin/lisp
