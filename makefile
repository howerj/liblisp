CC=gcc
CFLAGS=-Wall -Wextra -g -fwrapv -std=c99 -O2 -pedantic
TARGET=lisp
.PHONY: all clean documentation valgrind run libline/libline.a
all: $(TARGET)
doc: $(TARGET).htm 

lib$(TARGET).a: lib$(TARGET).o
	ar rcs $@ $<

lib$(TARGET).so: lib$(TARGET).c lib$(TARGET).h makefile
	$(CC) $(CFLAGS) lib$(TARGET).c -c -fpic -o $@
	$(CC) -shared -fPIC $< -o $@

lib$(TARGET).o: lib$(TARGET).c lib$(TARGET).h makefile
	$(CC) $(CFLAGS) $< -c -o $@

main.o: main.c lib$(TARGET).h makefile
	$(CC) $(CFLAGS) -DUSE_LINE -DUSE_MATH $< -c -o $@

$(TARGET): main.o lib$(TARGET).a libline/libline.a 
	$(CC) $(CFLAGS) -lm $^ -o $@

libline/libline.a:
	cd libline && make

$(TARGET).htm: $(TARGET).md
	markdown $^ > $@
run: $(TARGET)
	./$^ -Epc init.lsp test.lsp -
valgrind: $(TARGET)
	valgrind --leak-check=full ./$^ -Epc init.lsp test.lsp -
doxygen: doxygen.conf *.c *.h
	doxygen $^
liblisp.htm: liblisp.md
	markdown $^ > $@
documentation: doxygen liblisp.htm
clean:
	cd libline && make clean
	rm -rf $(TARGET) *.a *.so *.o *.log *.htm *.html *.out doxygen
