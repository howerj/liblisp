CC=gcc
CFLAGS=-Wall -Wextra -g -fwrapv -std=c99 -O2 -pedantic
TARGET=lisp
.PHONY: all clean doc valgrind run libline/libline.a
all: $(TARGET)
doc: lib$(TARGET).htm doxygen

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

lib$(TARGET).htm: lib$(TARGET).md
	markdown $^ > $@

run: $(TARGET)
	./$^ -Epc init.lsp test.lsp -

valgrind: $(TARGET)
	valgrind --leak-check=full ./$^ -Epc init.lsp test.lsp -

doxygen: doxygen.conf *.c *.h
	doxygen $^

clean:
	if [ -f libline/makefile ]; then cd libline && make clean; fi
	rm -rf $(TARGET) *.a *.so *.o *.log *.htm *.html *.out doxygen
