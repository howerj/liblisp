# liblisp
# A lisp library and interpreter
# Released under the LGPL license
#
# This requires GNU make to build, while compilation can be achieved with the
# Tiny C Compiler {all of the project, including modules, can be built using
# it}, only GCC and Clang work with *this* makefile.
#
# This makefile will build under Linux {Debian 7 and 8} and Windows 
# {Tested on Windows 7}.
#

include config.mk

TARGET = lisp
.PHONY: all clean dist doc doxygen valgrind run modules test

all: shorthelp $(TARGET) lib$(TARGET).$(DLL) lib$(TARGET).a modules unit

shorthelp:
	@echo "Use 'make help' for a list of options."

help:
	@echo ""
	@echo "project:     lib$(TARGET)"
	@echo "description: A small lisp library and example implementation"
	@echo "version:     $(VERSION)"
	@echo "commit:      $(VCS_COMMIT)"
	@echo "origin:      $(VCS_ORIGIN)"
	@echo "target:      $(TARGET_SYSTEM)"
	@echo ""
	@echo "If this makefile fails to build the project a simple interpreter,"
	@echo "lacking some features, can be built with:"
	@echo ""
	@echo "    cc *.c -o lisp"
	@echo ""
	@echo "The C compiler must support C99. This makefile requires GNU make and"
	@echo "should compile under Linux and Windows (given make and gcc are installed)"
	@echo ""
	@echo "make (option)*"
	@echo ""
	@echo "     all         build the example example executable and static library"
	@echo "     dist        make a distribution tar.gz file"
	@echo "     install     install the example executable, library and man pages"
	@echo "     uninstall   uninstall the example executable, library and man pages"
	@echo "     $(TARGET)        build the example executable"
	@echo "     lib$(TARGET).$(DLL)  build the library (dynamic)"
	@echo "     lib$(TARGET).a   build the library (static)"
	@echo "     modules     make as many modules as is possible (ignoring failures)"
	@echo "     run         make the example executable and run it"
	@echo "     help        this help message"
	@echo "     valgrind    make the example executable and run it with valgrind"
	@echo "     clean       remove all build artifacts and targets"
	@echo "     doc         make html and doxygen documentation"
	@echo "     TAGS        update project tags table (ctags)"
	@echo "     indent      indent the source code sensibly (instead of what I like)"
	@echo "     unit        executable for performing unit tests on liblisp"
	@echo "     test	run the unit tests"
	@echo ""

### building #################################################################

OBJFILES=hash.o io.o util.o gc.o read.o print.o subr.o\
	repl.o eval.o lisp.o tr.o valid.o compile.o

lib$(TARGET).a: $(OBJFILES)
	$(AR) $(AR_FLAGS) $@ $^

lib$(TARGET).$(DLL): $(OBJFILES) lib$(TARGET).h private.h
	$(CC) $(CFLAGS) -shared $(OBJFILES) -o $@

# -DCOMPILING_LIBLISP is only needed on Windows
%.o: %.c lib$(TARGET).h private.h
	$(CC) $(CFLAGS) -DCOMPILING_LIBLISP $< -c -o $@

VCS_DEFINES=-DVCS_ORIGIN="$(VCS_ORIGIN)" -DVCS_COMMIT="$(VCS_COMMIT)" -DVERSION="$(VERSION)" 

main.o: main.c lib$(TARGET).h 
	$(CC) $(CFLAGS_RELAXED) $(DEFINES) $(VCS_DEFINES) $< -c -o $@

$(TARGET): main.o lib$(TARGET).$(DLL)
	$(CC) $(CFLAGS) -Wl,-E $(RPATH) $^ $(LINK) -o $@

unit: unit.c lib$(TARGET).$(DLL)
	$(CC) $(CFLAGS) $(RPATH) $^ -o $@

test: unit
	./unit

# @bug recursive make considered harmful!
# Make as many modules as is possible
modules: lib$(TARGET).$(DLL) $(TARGET)
	-make -kC mod

### running ##################################################################

run: all
	@echo running the executable with default arguments
	./$(TARGET) $(RUN_FLAGS) init.lsp -

valgrind: all
	@echo running the executable through leak detection program, valgrind
	valgrind ./$(TARGET) $(RUN_FLAGS) init.lsp -

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
TARBALL=$(subst $(space),,$(TARGET)-$(VERSION).tgz) # Remove spaces 
# make distribution tarball
# @bug this currently creates a "tarbomb"
# @bug does not work on Windows {only Linux}
dist: $(TARGET) lib$(TARGET).a lib$(TARGET).$(DLL) lib$(TARGET).htm modules
	tar -zcf $(TARBALL) $(TARGET) *.htm *.$(DLL) *.a *.1 *.3


# @bug does not work on Windows {only Linux}, config.mk needs updating
install: all 
	$(CP) -f $(TARGET) $(DESTDIR)$(PREFIX)/bin
	$(MKDIR) $(MKDIR_FLAGS) $(DESTDIR)$(PREFIX)/bin
	$(MKDIR) $(MKDIR_FLAGS) $(DESTDIR)$(MANPREFIX)/man1
	$(MKDIR) $(MKDIR_FLAGS) $(DESTDIR)$(MANPREFIX)/man3
	$(MKDIR) $(MKDIR_FLAGS) $(DESTDIR)$(PREFIX)/lib
	$(SED) "s/VERSION/$(VERSION)/g" < $(TARGET).1    > $(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1
	$(SED) "s/VERSION/$(VERSION)/g" < lib$(TARGET).3 > $(DESTDIR)$(MANPREFIX)/man3/lib$(TARGET).3
	$(CP) $(CP_FLAGS) lib$(TARGET).a $(DESTDIR)$(PREFIX)/lib
	$(CP) $(CP_FLAGS) lib$(TARGET).$(DLL) $(DESTDIR)$(PREFIX)/lib
	$(CP) $(CP_FLAGS) lib$(TARGET).h $(DESTDIR)$(PREFIX)/include
	$(CHMOD) 755 $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	$(CHMOD) 644 $(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1
	$(CHMOD) 644 $(DESTDIR)$(MANPREFIX)/man3/lib$(TARGET).3
	$(CHMOD) 755 $(DESTDIR)$(PREFIX)/lib/lib$(TARGET).a
	$(CHMOD) 755 $(DESTDIR)$(PREFIX)/lib/lib$(TARGET).$(DLL)
	$(CHMOD) 644 $(DESTDIR)$(PREFIX)/include/lib$(TARGET).h
	$(LDCONFIG)
	@echo "installation complete"

uninstall:
	@echo uninstalling $(TARGET)
	@$(RM) -vf $(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1
	@$(RM) -vf $(DESTDIR)$(PREFIX)/bin/$(TARGET)
	@$(RM) -vf $(DESTDIR)$(PREFIX)/lib/lib$(TARGET).a
	@$(RM) -vf $(DESTDIR)$(PREFIX)/lib/lib$(TARGET).$(DLL)
	@$(RM) -vf $(DESTDIR)$(PREFIX)/include/lib$(TARGET).h

TAGS:
	ctags -R .

indent:
	indent -linux -l 110 *.c *.h */*.c */*.h
	$(RM) $(RM_FLAGS) *~ 
	$(RM) $(RM_FLAGS) */*~

### clean up #################################################################

clean:
	@echo Cleaning repository.
	-$(RM) $(RM_FLAGS) *.a 
	-$(RM) $(RM_FLAGS) *.$(DLL) 
	-$(RM) $(RM_FLAGS) *.o 
	-$(RM) $(RM_FLAGS) *.db
	-$(RM) $(RM_FLAGS) *.htm 
	-$(RM) $(RM_FLAGS) Doxyfile 
	-$(RM) $(RM_FLAGS) $(TARGET) 
	-$(RM) $(RM_FLAGS) *.tgz 
	-$(RM) $(RM_FLAGS) *~ 
	-$(RM) $(RM_FLAGS) */*~ 
	-$(RM) $(RM_FLAGS) *.log 
	-$(RM) $(RM_FLAGS) *.out 
	-$(RM) $(RM_FLAGS) tags
	-$(RM) $(RM_FLAGS) html/ 
	-$(RM) $(RM_FLAGS) latex/ 

##############################################################################

