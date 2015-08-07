CC=gcc
CFLAGS=-Wall -Wextra -g -fwrapv -std=c99 -pedantic -O2
TARGET=lisp
.PHONY: all clean doc doxygen valgrind run libline/libline.a
all: $(TARGET)
doc: lib$(TARGET).htm doxygen

lib$(TARGET).a: lib$(TARGET).o
	ar rcs $@ $<

lib$(TARGET).so: lib$(TARGET).c lib$(TARGET).h makefile
	$(CC) $(CFLAGS) lib$(TARGET).c -c -fpic -o $@
	$(CC) -shared -fPIC $< -o $@

lib$(TARGET).o: lib$(TARGET).c lib$(TARGET).h makefile
	$(CC) $(CFLAGS) $< -c -o $@

main.o: main.c lib$(TARGET).h makefile libline/libline.a libline/libline.h
	$(CC) $(CFLAGS) -DUSE_LINE -DUSE_MATH $< -c -o $@

$(TARGET): main.o lib$(TARGET).a libline/libline.a 
	$(CC) $(CFLAGS) -lm $^ -o $@

# Work around so the makefile initializes submodules. This requires
# the full liblisp repository to be available.
libline/.git:
	git submodule init
	git submodule update

libline/libline.h: libline/.git
libline/libline.a: libline/.git
	cd libline && make

lib$(TARGET).htm: lib$(TARGET).md
	markdown $^ > $@

run: $(TARGET)
	./$^ -Epc init.lsp test.lsp -

valgrind: $(TARGET)
	valgrind --leak-check=full ./$^ -Epc init.lsp test.lsp -

doxygen: 
	doxygen -g # makes Doxyfile
	doxygen Doxyfile

clean:
	if [ -f libline/makefile ]; then cd libline && make clean; fi
	rm -rf $(TARGET) *.a *.so *.o 
	rm -rf html latex *.bak doxygen *.htm *.html 
	rm -rf .list
	rm -rf *.log *.out *.bak *~

help:
	@echo "make [option]"
	@echo "     all         build the example lisp interpreter"
	@echo "     lisp        build the example lisp interpreter"
	@echo "     liblisp.so  build the lisp library"
	@echo "     liblisp.a   build the lisp library (static)"
	@echo "     run         make the interpreter and run it"
	@echo "     help        this help message"
	@echo "     valgrind    make the interpreter and run it with valgrind"
	@echo "     clean       remove all build artifacts and targets"
	@echo "     doc         make html and doxygen documentation"

