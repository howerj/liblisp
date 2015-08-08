# liblisp
# A lisp library and interpreter
# Released under the LGPL license
#
# If this makefile fails for some reason the following commands
# should work:
#  
# 	cc liblisp.c -c -o liblisp.o
# 	ar rcs liblisp.a liblisp.o
# 	cc main.c -c -o main.o
# 	cc main.o liblisp.a -o lisp
#
# Which will build the library and the example interpreter.
#

include config.mk

TARGET = lisp
.PHONY: all clean doc doxygen valgrind run libline/libline.a
all: $(TARGET)
doc: lib$(TARGET).htm doxygen

lib$(TARGET).a: lib$(TARGET).o
	ar rcs $@ $<

lib$(TARGET).so: lib$(TARGET).c lib$(TARGET).h 
	$(CC) $(CFLAGS) lib$(TARGET).c -c -fpic -o $@
	$(CC) -shared -fPIC $< -o $@

lib$(TARGET).o: lib$(TARGET).c lib$(TARGET).h 
	$(CC) $(CFLAGS) $< -c -o $@

main.o: main.c lib$(TARGET).h makefile libline/libline.a libline/libline.h
	$(CC) $(CFLAGS) ${DEFINES} $< -c -o $@

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
	doxygen -g 
	doxygen Doxyfile

clean:
	if [ -f libline/makefile ]; then cd libline && make clean; fi
	rm -rf $(TARGET) *.a *.so *.o 
	rm -rf html latex *.bak doxygen *.htm *.html Doxyfile
	rm -rf .list
	rm -rf *.log *.out *.bak *~

install: all
	@echo "installing executable ${TARGET} to ${DESTDIR}${PREFIX}/bin"
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${TARGET} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${TARGET}
	@echo "installing manual pages to ${DESTDIR}${MANPREFIX}/man1"
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	sed "s/VERSION/${VERSION}/g" < ${TARGET}.1 > ${DESTDIR}${MANPREFIX}/man1/${TARGET}.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/${TARGET}.1

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

