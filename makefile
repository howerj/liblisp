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
MAKEFLAGS += --no-builtin-rules

.SUFFIXES:
.PHONY: all clean dist doc doxygen valgrind run modules test

include config.mk

TARGET = lisp

all: shorthelp $(TARGET)$(EXE) lib$(TARGET).$(DLL) lib$(TARGET).a modules unit$(EXE)

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

$(TARGET)$(EXE): main.o lib$(TARGET).$(DLL)
	$(CC) $(CFLAGS) -Wl,-E $(RPATH) $^ $(LINK) -o $(TARGET)

unit$(EXE): unit.c lib$(TARGET).$(DLL)
	$(CC) $(CFLAGS) $(RPATH) $^ -o unit$(EXE)

test: unit$(EXE)
	./unit -c

# Make as many modules as is possible
# @bug recursive make considered harmful!
modules: lib$(TARGET).$(DLL) $(TARGET)$(EXE)
	-make -kC mod

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

# "space :=" and "space +=" are a hack along with "subst" to stop
# 'make' adding spaces to things when it should not.
space := 
space +=
TARBALL=$(subst $(space),,$(TARGET)-$(VERSION).tgz) # Remove spaces 
# make distribution tarball
# @bug this currently creates a "tarbomb"
dist: $(TARGET) lib$(TARGET).a lib$(TARGET).$(DLL) lib$(TARGET).htm modules
	tar -zcf $(TARBALL) $(TARGET) *.htm *.$(DLL) *.a *.1 *.3

install: all 
	-$(MKDIR) $(MKDIR_FLAGS) $(DESTDIR)$(PREFIX)$(FS)bin
	-$(MKDIR) $(MKDIR_FLAGS) $(DESTDIR)$(PREFIX)$(FS)lib
	-$(MKDIR) $(MKDIR_FLAGS) $(DESTDIR)$(PREFIX)$(FS)include
	-$(MKDIR) $(MKDIR_FLAGS) $(DESTDIR)$(MANPREFIX)$(FS)man1
	-$(MKDIR) $(MKDIR_FLAGS) $(DESTDIR)$(MANPREFIX)$(FS)man3
	$(CP) $(CP_FLAGS) $(TARGET)$(EXE) $(DESTDIR)$(PREFIX)$(FS)bin
	$(SED) "s/VERSION/$(VERSION)/g" < $(TARGET).1    > $(DESTDIR)$(MANPREFIX)$(FS)man1$(FS)$(TARGET).1
	$(SED) "s/VERSION/$(VERSION)/g" < lib$(TARGET).3 > $(DESTDIR)$(MANPREFIX)$(FS)man3$(FS)lib$(TARGET).3
	$(CP) $(CP_FLAGS) lib$(TARGET).a $(DESTDIR)$(PREFIX)$(FS)lib
	$(CP) $(CP_FLAGS) lib$(TARGET).$(DLL) $(DESTDIR)$(PREFIX)$(FS)lib
	$(CP) $(CP_FLAGS) lib$(TARGET).h $(DESTDIR)$(PREFIX)$(FS)include
	$(CHMOD) 755 $(DESTDIR)$(PREFIX)$(FS)bin$(FS)$(TARGET)
	$(CHMOD) 644 $(DESTDIR)$(MANPREFIX)$(FS)man1$(FS)$(TARGET).1
	$(CHMOD) 644 $(DESTDIR)$(MANPREFIX)$(FS)man3$(FS)lib$(TARGET).3
	$(CHMOD) 755 $(DESTDIR)$(PREFIX)$(FS)lib$(FS)lib$(TARGET).a
	$(CHMOD) 755 $(DESTDIR)$(PREFIX)$(FS)lib$(FS)lib$(TARGET).$(DLL)
	$(CHMOD) 644 $(DESTDIR)$(PREFIX)$(FS)include$(FS)lib$(TARGET).h
	$(LDCONFIG)
	@echo "installation complete"

uninstall:
	@echo uninstalling $(TARGET)$(EXE)
	$(RM) $(RM_FLAGS) $(DESTDIR)$(MANPREFIX)$(FS)man1$(FS)$(TARGET).1
	$(RM) $(RM_FLAGS) $(DESTDIR)$(PREFIX)$(FS)bin$(FS)$(TARGET)$(EXE)
	$(RM) $(RM_FLAGS) $(DESTDIR)$(PREFIX)$(FS)lib$(FS)lib$(TARGET).a
	$(RM) $(RM_FLAGS) $(DESTDIR)$(PREFIX)$(FS)lib$(FS)lib$(TARGET).$(DLL)
	$(RM) $(RM_FLAGS) $(DESTDIR)$(PREFIX)$(FS)include$(FS)lib$(TARGET).h

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

