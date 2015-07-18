.PHONY: all clean
CFLAGS+= -Wall -Wextra -ansi -pedantic -Wswitch-enum -Os -g 
TARGET=buffer_tb
CC=gcc

all: $(TARGET)

$(TARGET): $(TARGET).c buffer.o
	$(CC) $(CFLAGS) $^ -o $@

buffer.o: buffer.c buffer.h
	$(CC) $(CFLAGS) $< -c -o $@

clean:
	rm -f *.o $(TARGET)
