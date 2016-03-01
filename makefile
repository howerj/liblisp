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

##############################################################################
## Configuration and operating system options ################################
##############################################################################
# liblisp configuration and build system options
# @todo Replace this with a ./configure script (possibly). This will most
#       likely be written in perl.

# make run options
RUN_FLAGS=-Epc

# Version control variables and information
## These commands will depend on what version control is being run, or
## if any is being used at all. Currently git is being used.
## @todo These should all be set to unknown if the repository ".git/"
##       is unavailable
VERSION    =$(shell git describe) 
VCS_COMMIT =$(shell git rev-parse --verify HEAD)
VCS_ORIGIN =$(shell git config --get remote.origin.url)

CC= gcc
# The CFLAGS_RELAXED is used to compile main.c, main.c uses
# several libraries that require a cast from "void*" to a
# function pointer, which causes warnings which are unnecessary.
CFLAGS_RELAXED = -Wall -Wextra -g -fwrapv -O2 -Wmissing-prototypes
CFLAGS 	= $(CFLAGS_RELAXED) -pedantic
# Add the following to CFLAGS_RELAXED:
#  -fprofile-arcs -ftest-coverage
# For profiling information, see:
#  <https://gcc.gnu.org/onlinedocs/gcc/Gcov.html>

# Compilation options

## CPP defines of use
#   NDEBUG       Disable asserts
#   USE_DL	 Add support for dlopen/LoadLibrary, requires "-ldl" 
#                on Unix systems
#   USE_INITRC   Add support for a per user login file located in
#                ~/.lisprc which is run before the interpreter is.
#   USE_ABORT_HANDLER This adds in a handler that catches SIGABRT
#                and prints out a stack trace if it can.
DEFINES = -DUSE_DL -DUSE_INITRC -DUSE_ABORT_HANDLER
LINK    = -ldl
# This is for convenience only, it may cause problems.
RPATH   ?= -Wl,-rpath=. -Wl,-rpath=./mod 

ifeq ($(OS),Windows_NT)
FixPath =$(subst /,\,$1)
FS      =$(subst /,\,/)
AR       = ar
AR_FLAGS = rcs
DESTDIR =C:$(FS)
prefix  =lisp
MANPREFIX =$(prefix)$(FS)share$(FS)man
RM	=del
RM_FLAGS= /Q
LDCONFIG=
CP	=copy
CP_FLAGS=
MV	=move
CHMOD	=echo
MKDIR_FLAGS=
RUN_FLAGS=-Ep
DLL	=dll
EXE	=.exe
LINKFLAGS=-Wl,-E 
else # Unix assumed {only Linux has been tested}
# Install paths
FixPath = $1
FS        =/
DESTDIR   ?= 
prefix 	  ?=
MANPREFIX = $(call FixPath,$(prefix)/share/man)
# commands and their flags
AR       = ar
AR_FLAGS = rcs
RM    	 = rm
RM_FLAGS = -rf
CP       = cp
CP_FLAGS = -f
MV       = mv
CHMOD    = chmod
MKDIR    = mkdir
MKDIR_FLAGS= -p
SED      = sed
LDCONFIG = ldconfig
# misc
DLL=so
EXE=

CFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fstack-protector 
CFLAGS += -fPIC
CFLAGS_RELAXED += -fPIC
LINKFLAGS=-Wl,-E -Wl,-z,relro
endif

CFLAGS += -std=c99

##############################################################################
### Main Makefile ############################################################
##############################################################################

TARGET = lisp

all: shorthelp $(TARGET)$(EXE) lib$(TARGET).$(DLL) lib$(TARGET).a unit$(EXE)

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
	@echo "     app         make a self contained app with all dependencies"
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
	$(CC) $(CFLAGS) $(LINKFLAGS) $(RPATH) $^ $(LINK) -o $(TARGET)

unit$(EXE): unit.c lib$(TARGET).a
	$(CC) $(CFLAGS) $(RPATH) $^ -o unit$(EXE)

test: unit$(EXE)
	./unit -c

app: all test modules
	./app -vpa  ./lisp -e -Epc '"$${SCRIPT_PATH}"/data/init.lsp'

### running ##################################################################

run: all
	@echo running the executable with default arguments
	./$(TARGET) $(RUN_FLAGS) data/init.lsp -

# From <http://valgrind.org/>
valgrind: all
	@echo running the executable through leak detection program, valgrind
	valgrind ./$(TARGET) $(RUN_FLAGS) data/init.lsp -

### documentation ############################################################

# From <https://daringfireball.net/projects/markdown/>
lib$(TARGET).htm: lib$(TARGET).md
	markdown $^ > $@

doc: lib$(TARGET).htm doxygen

# From <http://www.stack.nl/~dimitri/doxygen/>
doxygen: 
	#doxygen -g  # old method
	#doxygen Doxyfile 2> doxygen.log
	doxygen doxygen.conf

### distribution and installation ############################################

TARBALL=$(strip $(TARGET)-$(VERSION)).tgz
# make distribution tarball
# @bug this currently creates a "tarbomb"
dist: $(TARGET) lib$(TARGET).a lib$(TARGET).$(DLL) lib$(TARGET).htm lib$(TARGET).h modules
	tar -zcf $(TARBALL) $(TARGET) *.htm *.$(DLL) lib$(TARGET).h *.a *.1 *.3 

install: all 
	-$(MKDIR) $(MKDIR_FLAGS) $(call FixPath,$(DESTDIR)$(prefix)/bin)
	-$(MKDIR) $(MKDIR_FLAGS) $(call FixPath,$(DESTDIR)$(prefix)/lib)
	-$(MKDIR) $(MKDIR_FLAGS) $(call FixPath,$(DESTDIR)$(prefix)/include)
	-$(MKDIR) $(MKDIR_FLAGS) $(call FixPath,$(DESTDIR)$(MANPREFIX)/man1)
	-$(MKDIR) $(MKDIR_FLAGS) $(call FixPath,$(DESTDIR)$(MANPREFIX)/man3)
	$(CP) $(CP_FLAGS) $(TARGET)$(EXE) $(call FixPath,$(DESTDIR)$(prefix)/bin)
	$(CP) $(CP_FLAGS) *.a $(call FixPath,$(DESTDIR)$(prefix)/lib)
	$(CP) $(CP_FLAGS) *.$(DLL) $(call FixPath,$(DESTDIR)$(prefix)/lib)
	$(CP) $(CP_FLAGS) lib$(TARGET).h $(call FixPath,$(DESTDIR)$(prefix)/include)
	$(SED) "s/VERSION/$(VERSION)/g" < $(TARGET).1    > $(call FixPath,$(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1)
	$(SED) "s/VERSION/$(VERSION)/g" < lib$(TARGET).3 > $(call FixPath,$(DESTDIR)$(MANPREFIX)/man3/lib$(TARGET).3)
	$(CHMOD) 755 $(call FixPath,$(DESTDIR)$(prefix)/bin/$(TARGET))
	$(CHMOD) 644 $(call FixPath,$(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1)
	$(CHMOD) 644 $(call FixPath,$(DESTDIR)$(MANPREFIX)/man3/lib$(TARGET).3)
	$(CHMOD) 755 $(call FixPath,$(DESTDIR)$(prefix)/lib/lib$(TARGET).a)
	$(CHMOD) 755 $(call FixPath,$(DESTDIR)$(prefix)/lib/lib$(TARGET).$(DLL))
	$(CHMOD) 644 $(call FixPath,$(DESTDIR)$(prefix)/include/lib$(TARGET).h)
	#$(LDCONFIG)
	@echo "installation complete"

uninstall:
	@echo uninstalling $(TARGET)$(EXE)
	$(RM) $(RM_FLAGS) $(call FixPath,$(DESTDIR)$(MANPREFIX)/man1/$(TARGET).1)
	$(RM) $(RM_FLAGS) $(call FixPath,$(DESTDIR)$(MANPREFIX)/man3/lib$(TARGET).3)
	$(RM) $(RM_FLAGS) $(call FixPath,$(DESTDIR)$(prefix)/bin/$(TARGET)$(EXE))
	$(RM) $(RM_FLAGS) $(call FixPath,$(DESTDIR)$(prefix)/lib/lib$(TARGET).a)
	$(RM) $(RM_FLAGS) $(call FixPath,$(DESTDIR)$(prefix)/lib/*.$(DLL))
	$(RM) $(RM_FLAGS) $(call FixPath,$(DESTDIR)$(prefix)/include/lib$(TARGET).h)

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
	-$(RM) $(RM_FLAGS) lisp-linux-*/ 

##############################################################################

