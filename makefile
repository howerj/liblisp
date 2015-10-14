# liblisp
# A lisp library and interpreter
# Released under the LGPL license
#
# It would be nice to have one portable makefile, but it is not to be so, this
# makefile requires GNU Make.
#

include config.mk

TARGET = lisp
.PHONY: all clean dist doc doxygen valgrind run 

all: help $(TARGET) lib$(TARGET).so

help:
	@echo ""
	@echo "project:     lib$(TARGET)"
	@echo "description: A small lisp library and example implementation"
	@echo "version:     $(VERSION)"
	@echo "commit:      $(VCS_COMMIT)"
	@echo "origin:      $(VCS_ORIGIN)"
	@echo ""
	@echo "If this makefile fails to build the project a simple interpreter,"
	@echo "lacking some features, can be built with:"
	@echo ""
	@echo "    cc *.c -o lisp"
	@echo ""
	@echo "The C compiler must support C99."
	@echo ""
	@echo "make (option)*"
	@echo ""
	@echo "     all         build the example example executable and static library"
	@echo "     dist        make a distribution tar.gz file"
	@echo "     install     install the example executable, library and man pages"
	@echo "     uninstall   uninstall the example executable, library and man pages"
	@echo "     $(TARGET)        build the example executable"
	@echo "     lib$(TARGET).so  build the library (dynamic)"
	@echo "     lib$(TARGET).a   build the library (static)"
	@echo "     run         make the example executable and run it"
	@echo "     help        this help message"
	@echo "     valgrind    make the example executable and run it with valgrind"
	@echo "     clean       remove all build artifacts and targets"
	@echo "     doc         make html and doxygen documentation"
	@echo "     TAGS        update project tags table (ctags)"
	@echo ""

### building #################################################################

OBJFILES=hash.o io.o util.o gc.o read.o print.o\
	 subr.o repl.o eval.o lisp.o tr.o valid.o

lib$(TARGET).a: $(OBJFILES)
	$(AR) $(AR_FLAGS) $@ $^

lib$(TARGET).so: $(OBJFILES) lib$(TARGET).h private.h
	$(CC) -shared -fPIC $(OBJFILES) -o $@
 
%.o: %.c lib$(TARGET).h private.h
	$(CC) $(CFLAGS) $< -c -o $@

VCS_DEFINES=-DVCS_ORIGIN="${VCS_ORIGIN}" -DVCS_COMMIT="${VCS_COMMIT}" -DVERSION="${VERSION}" 

main.o: main.c lib$(TARGET).h 
	$(CC) $(CFLAGS_RELAXED) ${DEFINES} ${VCS_DEFINES} $< -c -o $@

$(TARGET): main.o lib$(TARGET).a 
	$(CC) $(CFLAGS) -Wl,-E $^ -lm ${LINK} -o $@

### running ##################################################################

run: $(TARGET)
	@echo running the executable with default arguments
	$(PRELOAD) ./$^ -Epc init.lsp -

valgrind: $(TARGET)
	@echo running the executable through leak detection program, valgrind
	$(PRELOAD) valgrind ./$^ -Epc init.lsp -

### documentation ############################################################

lib$(TARGET).htm: lib$(TARGET).md
	markdown $^ > $@

doc: lib$(TARGET).htm doxygen

doxygen: 
	doxygen -g 
	doxygen Doxyfile 2> doxygen.log

### distribution and installation ############################################

# "space :=" and "space +=" are a hack along with "subst" to stop
# 'make' adding spaces to things when it should not.
space := 
space +=
TARBALL=$(subst $(space),,${TARGET}-${VERSION}.tgz) # Remove spaces 
# make distribution tarball
dist: ${TARGET} lib${TARGET}.[ah3] lib${TARGET}.so lib${TARGET}.htm ${TARGET}.1
	tar -zcf ${TARBALL} $^

install: all 
	@echo "installing executable ${TARGET} to ${DESTDIR}${PREFIX}/bin"
	$(CP) -f ${TARGET} ${DESTDIR}${PREFIX}/bin
	@echo "installing manual pages to ${DESTDIR}${MANPREFIX}/man1"
	$(MKDIR) -p ${DESTDIR}${PREFIX}/bin
	$(MKDIR) -p ${DESTDIR}${MANPREFIX}/man1
	$(MKDIR) -p ${DESTDIR}${PREFIX}/lib
	@echo "updating manual page version"
	sed "s/VERSION/${VERSION}/g" < ${TARGET}.1 > ${DESTDIR}${MANPREFIX}/man1/${TARGET}.1
	@echo "install library files"
	$(CP) -f lib${TARGET}.a ${DESTDIR}${PREFIX}/lib
	$(CP) -f lib${TARGET}.so ${DESTDIR}${PREFIX}/lib
	$(CP) -f lib${TARGET}.h ${DESTDIR}${PREFIX}/include
	$(CHMOD) 755 ${DESTDIR}${PREFIX}/bin/${TARGET}
	$(CHMOD) 644 ${DESTDIR}${MANPREFIX}/man1/${TARGET}.1
	$(CHMOD) 755 ${DESTDIR}${PREFIX}/lib/lib${TARGET}.a
	$(CHMOD) 755 ${DESTDIR}${PREFIX}/lib/lib${TARGET}.so
	$(CHMOD) 644 ${DESTDIR}${PREFIX}/include/lib${TARGET}.h
	$(LDCONFIG)
	@echo "installation complete"

uninstall:
	@echo uninstalling ${TARGET}
	@$(RM) -vf ${DESTDIR}${MANPREFIX}/man1/${TARGET}.1
	@$(RM) -vf ${DESTDIR}${PREFIX}/bin/${TARGET}
	@$(RM) -vf ${DESTDIR}${PREFIX}/lib/lib${TARGET}.a
	@$(RM) -vf ${DESTDIR}${PREFIX}/lib/lib${TARGET}.so
	@$(RM) -vf ${DESTDIR}${PREFIX}/include/lib${TARGET}.h

TAGS:
	ctags -R .

### clean up #################################################################

CLEAN_LIST = *.a *.so *.o *.htm Doxyfile $(TARGET) *.tgz *~ *.log *.out html/ latex/

clean:
	@echo Cleaning repository.
	$(RM) -rf $(CLEAN_LIST)

##############################################################################

