# Make file for the "linenoise" library, this will build an
# example so that you can test out the library

CC=gcc
CFLAGS=-Wall -W -Os -g
INDENT=-linux -nut -l 150
OBJFILES=llsp.o linenoise.o io.o mem.o sexpr.o lisp.o gc.o

#TARGET=example
TARGET=llsp

.PHONY: all indent run clean

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) -o $(TARGET)

%.o: %.c *.h makefile
	$(CC) -c $(CFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET)

indent:
	indent $(INDENT) *.c *.h
	rm *~

clean:
	rm -f *.o *~ $(TARGET)

