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
# This makefile should be changed so that it does not make use of recursive
# makes, examples of doing things the right way can be found:
# <https://stackoverflow.com/questions/559216/what-is-your-experience-with-non-recursive-make>
# and specifically:
# <https://github.com/dmoulding/boilermake>
MAKEFLAGS += --no-builtin-rules

.SUFFIXES:
.PHONY: all clean dist doc doxygen valgrind run test

include config.mk

TARGET = lisp

all: shorthelp $(TARGET)$(EXE) lib$(TARGET).$(DLL) lib$(TARGET).a modules unit$(EXE)

include mod/makefile.in

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
	@echo "     $(TARGET)$(EXE)      build the example executable"
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
	@echo "     unit$(EXE)  executable for performing unit tests on liblisp"
	@echo "     test	run the unit tests"
	@echo ""

### building #################################################################

OBJFILES=hash.o io.o util.o gc.o read.o print.o subr.o repl.o eval.o lisp.o valid.o 

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

$(TARGET)$(EXE): main.o lib$(TARGET).$(DLL)
	$(CC) $(CFLAGS) -Wl,-E $(RPATH) $^ $(LINK) -o $(TARGET)

unit$(EXE): unit.c lib$(TARGET).$(DLL)
	$(CC) $(CFLAGS) $(RPATH) $^ -o unit$(EXE)

test: unit$(EXE)
	./unit -c

### running ##################################################################

run: all
	@echo running the executable with default arguments
	./$(TARGET) $(RUN_FLAGS) init.lsp -

# From <http://valgrind.org/>
valgrind: all
	@echo running the executable through leak detection program, valgrind
	valgrind ./$(TARGET) $(RUN_FLAGS) init.lsp -

### documentation ############################################################

# From <https://daringfireball.net/projects/markdown/>
lib$(TARGET).htm: lib$(TARGET).md
	markdown $^ > $@

doc: lib$(TARGET).htm doxygen

# From <http://www.stack.nl/~dimitri/doxygen/>
doxygen: 
	doxygen -g 
	doxygen Doxyfile 2> doxygen.log

### distribution and installation ############################################

TARBALL=$(strip $(TARGET)-$(VERSION)).tgz
# make distribution tarball
# @bug this currently creates a "tarbomb"
dist: $(TARGET) lib$(TARGET).a lib$(TARGET).$(DLL) lib$(TARGET).htm lib$(TARGET).h modules
	tar -zcf $(TARBALL) $(TARGET) *.htm *.$(DLL) lib$(TARGET).h *.a *.1 *.3 

install: all 
	-$(MKDIR) $(MKDIR_FLAGS) $(call FixPath,$(DESTDIR)$(PREFIX)/bin)
	-$(MKDIR) $(MKDIR_FLAGS) $(call FixPath,$(DESTDIR)$(PREFIX)/lib)
	-$(MKDIR) $(MKDIR_FLAGS) $(call FixPath,$(DESTDIR)$(PREFIX)/include)
	-$(MKDIR) $(MKDIR_FLAGS) $(call FixPath,$(DESTDIR)$(MANPREFIX)/man1)
	-$(MKDIR) $(MKDIR_FLAGS) $(call FixPath,$(DESTDIR)$(MANPREFIX)/man3)
	$(CP) $(CP_FLAGS) $(TARGET)$(EXE) $(call FixPath,$(DESTDIR)$(PREFIX)/bin)
	$(CP) $(CP_FLAGS) *.a $(call FixPath,$(DESTDIR)$(PREFIX)/lib)
	$(CP) $(CP_FLAGS) *.$(DLL) $(call FixPath,$(DESTDIR)$(PREFIX)/lib)
	$(CP) $(CP_FLAGS) lib$(TARGET).h $(call FixPath,$(DESTDIR)$(PREFIX)/include)
	$(SED) "s/VERSION/$(VERSION)/g" < $(TARGET).1    > $(call FixPath,$(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1)
	$(SED) "s/VERSION/$(VERSION)/g" < lib$(TARGET).3 > $(call FixPath,$(DESTDIR)$(MANPREFIX)/man3/lib$(TARGET).3)
	$(CHMOD) 755 $(call FixPath,$(DESTDIR)$(PREFIX)/bin/$(TARGET))
	$(CHMOD) 644 $(call FixPath,$(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1)
	$(CHMOD) 644 $(call FixPath,$(DESTDIR)$(MANPREFIX)/man3/lib$(TARGET).3)
	$(CHMOD) 755 $(call FixPath,$(DESTDIR)$(PREFIX)/lib/lib$(TARGET).a)
	$(CHMOD) 755 $(call FixPath,$(DESTDIR)$(PREFIX)/lib/lib$(TARGET).$(DLL))
	$(CHMOD) 644 $(call FixPath,$(DESTDIR)$(PREFIX)/include/lib$(TARGET).h)
	$(LDCONFIG)
	@echo "installation complete"

uninstall:
	@echo uninstalling $(TARGET)$(EXE)
	$(RM) $(RM_FLAGS) $(call FixPath,$(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1)
	$(RM) $(RM_FLAGS) $(call FixPath,$(DESTDIR)$(MANPREFIX)/man3/lib$(TARGET).3)
	$(RM) $(RM_FLAGS) $(call FixPath,$(DESTDIR)$(PREFIX)/bin/$(TARGET)$(EXE))
	$(RM) $(RM_FLAGS) $(call FixPath,$(DESTDIR)$(PREFIX)/lib/lib$(TARGET).a)
	$(RM) $(RM_FLAGS) $(call FixPath,$(DESTDIR)$(PREFIX)/lib/*.$(DLL))
	$(RM) $(RM_FLAGS) $(call FixPath,$(DESTDIR)$(PREFIX)/include/lib$(TARGET).h)

# From <http://ctags.sourceforge.net/>
TAGS:
	ctags -R .

# From <https://www.gnu.org/software/indent/>
indent:
	indent -linux -l 110 *.c *.h */*.c */*.h
	$(RM) $(RM_FLAGS) *~ 
	$(RM) $(RM_FLAGS) *$(FS)*~

### clean up #################################################################

clean:
	@echo Cleaning repository.
	-$(RM) $(RM_FLAGS) $(TARGET)$(EXE)
	-$(RM) $(RM_FLAGS) unit$(EXE)
	-$(RM) $(RM_FLAGS) *.$(DLL) 
	-$(RM) $(RM_FLAGS) *.a 
	-$(RM) $(RM_FLAGS) *.o 
	-$(RM) $(RM_FLAGS) *.db
	-$(RM) $(RM_FLAGS) *.htm 
	-$(RM) $(RM_FLAGS) Doxyfile 
	-$(RM) $(RM_FLAGS) *.tgz 
	-$(RM) $(RM_FLAGS) *~ 
	-$(RM) $(RM_FLAGS) */*~ 
	-$(RM) $(RM_FLAGS) *.log 
	-$(RM) $(RM_FLAGS) *.out 
	-$(RM) $(RM_FLAGS) *.bak
	-$(RM) $(RM_FLAGS) tags
	-$(RM) $(RM_FLAGS) html/ 
	-$(RM) $(RM_FLAGS) latex/ 

##############################################################################

