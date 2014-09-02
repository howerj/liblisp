# Make file for the "linenoise" library, this will build an
# example so that you can test out the library

CC=gcc
CFLAGS=-Wall -W -Os -g
OBJFILES=example.o linenoise.o
TARGET=example

.PHONY: all indent run clean

all: $(TARGET)

$(TARGET): $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) -o example 

%.o: %.c *.h makefile
	$(CC) -c $(CFLAGS) -o $@ $<

run: $(TARGET)
	./$(TARGET)

indent:
	indent $(INDENT) *.c *.h
	rm *~

clean:
	rm -f *.o *~ $(TARGET)

