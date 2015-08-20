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
.PHONY: all clean dist doc doxygen valgrind run libline/libline.a

all: $(TARGET)

help:
	@echo ""
	@echo "project:     lib${TARGET}"
	@echo "description: A small lisp library and example implementation"
	@echo "version:     $(VERSION)"
	@echo "commit:      $(VCS_COMMIT)"
	@echo "origin:      $(VCS_ORIGIN)"
	@echo ""
	@echo "make (option)*"
	@echo ""
	@echo "     all         build the example example executable and static library"
	@echo "     dist        make a distribution tar.gz file"
	@echo "     install     install the example executable, library and man pages"
	@echo "     uninstall   uninstall the example executable, library and man pages"
	@echo "     ${TARGET}        build the example executable"
	@echo "     lib${TARGET}.so  build the library (dynamic)"
	@echo "     lib${TARGET}.a   build the library (static)"
	@echo "     run         make the example executable and run it"
	@echo "     help        this help message"
	@echo "     valgrind    make the example executable and run it with valgrind"
	@echo "     clean       remove all build artifacts and targets"
	@echo "     doc         make html and doxygen documentation"

### building #################################################################

lib$(TARGET).a: lib$(TARGET).o
	ar rcs $@ $<

lib$(TARGET).so: lib$(TARGET).c lib$(TARGET).h 
	$(CC) $(CFLAGS) lib$(TARGET).c -c -fpic -o $@
	$(CC) -shared -fPIC $< -o $@

lib$(TARGET).o: lib$(TARGET).c lib$(TARGET).h 
	$(CC) $(CFLAGS) $< -c -o $@

VCS_DEFINES=-DVCS_ORIGIN="${VCS_ORIGIN}" -DVCS_COMMIT="${VCS_COMMIT}" -DVERSION="${VERSION}" 
# Always rebuilds as libline.h is .PHONY, it has to be.
main.o: main.c lib$(TARGET).h libline/libline.h
	$(CC) $(CFLAGS) ${DEFINES} ${VCS_DEFINES} $< -c -o $@

# Always rebuilds as libline.a is .PHONY, it has to be.
$(TARGET): main.o lib$(TARGET).a libline/libline.a 
	$(CC) $(CFLAGS) -lm $^ -o $@

# Work around so the makefile initializes submodules. This requires
# the full liblisp git repository to be available.
# The repository is available at <https://github.com/howerj/liblisp>

libline/.git:
	git submodule init
	git submodule update

libline/libline.h: libline/.git
libline/libline.a: libline/.git
	cd libline && make

### running ##################################################################

run: $(TARGET)
	@echo running the executable with default arguments
	./$^ -Epc init.lsp -

valgrind: $(TARGET)
	@echo running the executable through leak detection program, valgrind
	valgrind --leak-check=full ./$^ -Epc init.lsp -

### documentation ############################################################

lib$(TARGET).htm: lib$(TARGET).md
	markdown $^ > $@

doc: lib$(TARGET).htm doxygen

doxygen: 
	doxygen -g 
	doxygen Doxyfile 2> doxygen.log

### distribution and installation ############################################

# "space :=" and "space +=" are a hack along with "subst" to stop
# make adding spaces to things when it should not.
space := 
space +=
TARBALL=$(subst $(space),,${TARGET}-${VERSION}.tgz) # Remove spaces 
# make distribution tarball
dist: ${TARGET} lib${TARGET}.[ah3] lib${TARGET}.so lib${TARGET}.htm ${TARGET}.1
	tar -zcf ${TARBALL} $^

install: all 
	@echo "installing executable ${TARGET} to ${DESTDIR}${PREFIX}/bin"
	mkdir -p ${DESTDIR}${PREFIX}/bin
	cp -f ${TARGET} ${DESTDIR}${PREFIX}/bin
	chmod 755 ${DESTDIR}${PREFIX}/bin/${TARGET}
	@echo "installing manual pages to ${DESTDIR}${MANPREFIX}/man1"
	mkdir -p ${DESTDIR}${MANPREFIX}/man1
	@echo "updating manual page version"
	sed "s/VERSION/${VERSION}/g" < ${TARGET}.1 > ${DESTDIR}${MANPREFIX}/man1/${TARGET}.1
	chmod 644 ${DESTDIR}${MANPREFIX}/man1/${TARGET}.1
	@echo "install library files"
	mkdir -p ${DESTDIR}${PREFIX}/lib
	cp -f lib${TARGET}.a ${DESTDIR}${PREFIX}/lib
	chmod 755 ${DESTDIR}${PREFIX}/lib/lib${TARGET}.a
	cp -f lib${TARGET}.h ${DESTDIR}${PREFIX}/include
	chmod 644 ${DESTDIR}${PREFIX}/include/lib${TARGET}.h
	@echo "installation complete"

uninstall:
	@echo uninstalling ${TARGET}
	@rm -vf ${DESTDIR}${MANPREFIX}/man1/${TARGET}.1
	@rm -vf ${DESTDIR}${PREFIX}/bin/${TARGET}
	@rm -vf ${DESTDIR}${PREFIX}/lib/lib${TARGET}.a
	@rm -vf ${DESTDIR}${PREFIX}/include/lib${TARGET}.h

### clean up #################################################################

clean:
	@echo Cleaning repository.
	if [ -f libline/makefile ]; then cd libline && make clean; fi
	rm -rf $(TARGET) *.a *.so *.o 
	rm -rf html latex *.bak doxygen *.htm *.html Doxyfile
	rm -rf .list
	rm -rf *.log *.out *.bak *.tgz *~

##############################################################################
