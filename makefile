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
# I should also use foreach loops, see
# <https://stackoverflow.com/questions/4134764/how-to-define-several-include-path-in-makefile>
#
# @todo Move the binary files to a bin/ directory
# @todo Make a makefile.in for src/
# @todo The build options USE_MUTEX, USE_ABORT_HANDLER, USE_DL should be merged.
#
MAKEFLAGS += --no-builtin-rules --keep-going

.SUFFIXES:
.PHONY: all clean dist doc doxygen valgrind run test

##############################################################################
## Configuration and operating system options ################################
##############################################################################
# liblisp configuration and build system options

# make run options

VERSION    =$(shell git describe) 
VCS_COMMIT =$(shell git rev-parse --verify HEAD)
VCS_ORIGIN =$(shell git config --get remote.origin.url)
VCS_DEFINES=-DVCS_ORIGIN="${VCS_ORIGIN}" -DVCS_COMMIT="${VCS_COMMIT}" -DVERSION="${VERSION}" 

CC := gcc
CFLAGS_RELAXED := -Wall -Wextra -g -fwrapv -O2 -Wmissing-prototypes
CFLAGS := $(CFLAGS_RELAXED) -pedantic

# Compilation options
## CPP defines of use
#   NDEBUG       Disable asserts
#   @todo Merge USE_DL and USE_MUTEX
#   USE_DL	 Add support for dlopen/LoadLibrary, requires "-ldl" 
#                on Unix systems
#   USE_MUTEX    Add support for Mutexes/Critical Sections, requires
#                "-lpthread" on Unix systems
#   USE_ABORT_HANDLER This adds in a handler that catches SIGABRT
#                and prints out a stack trace if it can.
DEFINES = -DUSE_DL -DUSE_ABORT_HANDLER -DUSE_MUTEX $(VCS_DEFINES)
# This is for convenience only, it may cause problems.
RPATH   ?= -Wl,-rpath=.

ifeq ($(OS),Windows_NT)
COLOR   =
FixPath =$(subst /,\,$1)
FS      =$(subst /,\,/)
AR       := ar
AR_FLAGS := rcs
DESTDIR :=C:${FS}
prefix  :=lisp
MANPREFIX :=${prefix}${FS}share${FS}man
RM	:=del
RM_FLAGS:= /Q
LDCONFIG:=
CP	:=copy
CP_FLAGS:=
MV	:=move
CHMOD	:=echo
MKDIR_FLAGS:=
DLL	:=dll
EXE	:=.exe
LINKFLAGS:=-Wl,-E 
SRC=src
else # Unix assumed {only Linux has been tested}
# Install paths
FixPath   = $1
FS        =/
DESTDIR   ?= 
prefix 	  ?=
MANPREFIX = $(call FixPath,${prefix}/share/man)
# commands and their flags
AR       :=ar
AR_FLAGS :=rcs
RM    	 :=rm
RM_FLAGS :=-rfv
CP       :=cp
CP_FLAGS :=-f
MV       :=mv
CHMOD    :=chmod
MKDIR    :=mkdir
MKDIR_FLAGS:=-p
SED      :=sed
LDCONFIG :=ldconfig
LINK     :=-ldl -lpthread
DLL      :=so
EXE      :=

COLOR = -c
CFLAGS += -U_FORTIFY_SOURCE -D_FORTIFY_SOURCE=2 -fstack-protector -fPIC
CFLAGS_RELAXED += -fPIC
LINKFLAGS=-Wl,-E -Wl,-z,relro
SRC=src
endif

RUN_FLAGS:=-Ep ${COLOR}
INCLUDE = -I${SRC}
CFLAGS += -std=c99
DOC=doc${FS}

VALGRIND_SUPPRESSION_FILE=${DOC}valgrind.supp
VALGRIND_OPTIONS=--leak-resolution=high --num-callers=40 --leak-check=full --log-file=valgrind.log

##############################################################################
### Main Makefile ############################################################
##############################################################################

TARGET = lisp

all: shorthelp ${TARGET}${EXE} lib${TARGET}.${DLL} lib${TARGET}.a 

include ${SRC}${FS}mod/makefile.in

shorthelp:
	@echo "Use 'make help' for a list of options."

help:
	@echo ""
	@echo "project:     lib${TARGET}"
	@echo "description: A small lisp library and example implementation"
	@echo "version:     ${VERSION}"
	@echo "commit:      ${VCS_COMMIT}"
	@echo "origin:      ${VCS_ORIGIN}"
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
	@echo "     ${TARGET}${EXE}      build the example executable"
	@echo "     lib${TARGET}.${DLL}  build the library (dynamic)"
	@echo "     lib${TARGET}.a   build the library (static)"
	@echo "     modules     make as many modules as is possible (ignoring failures)"
	@echo "     run         make the example executable and run it"
	@echo "     app         make a self contained app with all dependencies"
	@echo "     help        this help message"
	@echo "     valgrind    make the example executable and run it with valgrind"
	@echo "     clean       remove all build artifacts and targets"
	@echo "     doc         make html and doxygen documentation"
	@echo "     TAGS        update project tags table (ctags)"
	@echo "     indent      indent the source code sensibly (instead of what I like)"
	@echo "     unit${EXE}  executable for performing unit tests on liblisp"
	@echo "     test	run the unit tests"
	@echo ""

### building #################################################################

SOURCES=$(wildcard ${SRC}/*.c)
OBJFILES=$(SOURCES:${SRC}/%.c=%.o)

lib${TARGET}.a: ${OBJFILES}
	@echo AR $@
	@$(AR) $(AR_FLAGS) $@ $^

lib${TARGET}.${DLL}: ${OBJFILES} ${SRC}${FS}lib${TARGET}.h ${SRC}${FS}private.h
	@echo CC -o $@
	@${CC} ${CFLAGS} -shared ${OBJFILES} -o $@

%.o: ${SRC}${FS}%.c ${SRC}${FS}lib${TARGET}.h ${SRC}${FS}private.h makefile
	@echo CC $<
	@${CC} ${CFLAGS} ${INCLUDE} -DCOMPILING_LIBLISP $< -c -o $@

repl.o: ${SRC}${FS}repl.c ${SRC}${FS}lib${TARGET}.h makefile
	@echo CC $<
	@${CC} ${CFLAGS} ${INCLUDE} ${DEFINES} -DCOMPILING_LIBLISP $< -c -o $@

main.o: ${SRC}${FS}main.c ${SRC}${FS}lib${TARGET}.h ${SRC}${FS}lispmod.h makefile
	@echo CC $<
	@${CC} $(CFLAGS_RELAXED) ${INCLUDE} ${DEFINES} $< -c -o $@

${TARGET}${EXE}: main.o lib${TARGET}.${DLL}
	@echo CC -o $@
	@${CC} ${CFLAGS} ${LINKFLAGS} ${RPATH} $^ ${LINK} -o ${TARGET}

unit${EXE}: ${SRC}${FS}t/${FS}unit.c lib${TARGET}.a
	@echo CC -o $@
	@${CC} ${CFLAGS} ${INCLUDE} ${RPATH} $^ -o unit${EXE}

test: unit${EXE}
	./unit ${COLOR}

app: all test modules
	${SRC}${FS}./app -vpa  ./lisp -f ${DOC} -f lsp -e -Epc '"$${SCRIPT_PATH}"/lsp/init.lsp'

### running ##################################################################

run: all
	@echo running the executable with default arguments
	./${TARGET} ${RUN_FLAGS} lsp/init.lsp -

valgrind: all
	@echo running the executable through leak detection program, valgrind
	valgrind ${VALGRIND_OPTIONS} --suppressions=${VALGRIND_SUPPRESSION_FILE} ./${TARGET} ${RUN_FLAGS} lsp/init.lsp -

### documentation ############################################################

lib${TARGET}.htm: ${DOC}lib${TARGET}.md
	markdown $^ > $@

doc: lib${TARGET}.htm doxygen

doxygen: 
	doxygen ${DOC}doxygen.conf

### distribution and installation ############################################

TARBALL=$(strip ${TARGET}-${VERSION}).tgz
# make distribution tarball
dist: ${TARGET} lib${TARGET}.a lib${TARGET}.${DLL} lib${TARGET}.htm ${SRC}${FS}lib${TARGET}.h modules
	tar -zcf ${TARBALL} ${TARGET} *.htm *.${DLL} ${SRC}${FS}lib${TARGET}.h *.a ${DOC}*.1 ${DOC}*.3 

install: all 
	-${MKDIR} ${MKDIR_FLAGS} $(call FixPath,${DESTDIR}${prefix}/bin)
	-${MKDIR} ${MKDIR_FLAGS} $(call FixPath,${DESTDIR}${prefix}/lib)
	-${MKDIR} ${MKDIR_FLAGS} $(call FixPath,${DESTDIR}${prefix}/include)
	-${MKDIR} ${MKDIR_FLAGS} $(call FixPath,${DESTDIR}${MANPREFIX}/man1)
	-${MKDIR} ${MKDIR_FLAGS} $(call FixPath,${DESTDIR}${MANPREFIX}/man3)
	${CP} $(CP_FLAGS) ${TARGET}${EXE} $(call FixPath,${DESTDIR}${prefix}/bin)
	${CP} $(CP_FLAGS) *.a $(call FixPath,${DESTDIR}${prefix}/lib)
	${CP} $(CP_FLAGS) *.${DLL} $(call FixPath,${DESTDIR}${prefix}/lib)
	${CP} $(CP_FLAGS) ${SRC}${FS}lib${TARGET}.h $(call FixPath,${DESTDIR}${prefix}/include)
	${CP} $(CP_FLAGS) ${SRC}${FS}lispmod.h $(call FixPath,${DESTDIR}${prefix}/include)
	${SED} "s/VERSION/${VERSION}/g" < ${DOC}${TARGET}.1 > $(call FixPath,${DESTDIR}${MANPREFIX}/man1/${TARGET}.1)
	${SED} "s/VERSION/${VERSION}/g" < ${DOC}lib${TARGET}.3 > $(call FixPath,${DESTDIR}${MANPREFIX}/man3/lib${TARGET}.3)
	${CHMOD} 755 $(call FixPath,${DESTDIR}${prefix}/bin/${TARGET})
	${CHMOD} 644 $(call FixPath,${DESTDIR}${MANPREFIX}/man1/${TARGET}.1)
	${CHMOD} 644 $(call FixPath,${DESTDIR}${MANPREFIX}/man3/lib${TARGET}.3)
	${CHMOD} 755 $(call FixPath,${DESTDIR}${prefix}/lib/lib${TARGET}.a)
	${CHMOD} 755 $(call FixPath,${DESTDIR}${prefix}/lib/lib${TARGET}.${DLL})
	${CHMOD} 644 $(call FixPath,${DESTDIR}${prefix}/include/lib${TARGET}.h)
	${CHMOD} 644 $(call FixPath,${DESTDIR}${prefix}/include/lispmod.h)
	#$(LDCONFIG)
	@echo "installation complete"

uninstall:
	@echo uninstalling ${TARGET}${EXE}
	${RM} ${RM_FLAGS} $(call FixPath,${DESTDIR}${MANPREFIX}/man1/${TARGET}.1)
	${RM} ${RM_FLAGS} $(call FixPath,${DESTDIR}${MANPREFIX}/man3/lib${TARGET}.3)
	${RM} ${RM_FLAGS} $(call FixPath,${DESTDIR}${prefix}/bin/${TARGET}${EXE})
	${RM} ${RM_FLAGS} $(call FixPath,${DESTDIR}${prefix}/lib/lib${TARGET}.a)
	${RM} ${RM_FLAGS} $(call FixPath,${DESTDIR}${prefix}/lib/*.${DLL})
	${RM} ${RM_FLAGS} $(call FixPath,${DESTDIR}${prefix}/include/lib${TARGET}.h)

# From <http://ctags.sourceforge.net/>
TAGS:
	ctags -R .

# From <https://www.gnu.org/software/indent/>
indent:
	indent -linux -l 110 *.c *.h */*.c */*.h
	${RM} ${RM_FLAGS} *~ 
	${RM} ${RM_FLAGS} *${FS}*~

### clean up #################################################################

CLEAN=unit${EXE} *.${DLL} *.a *.o *.db *.htm Doxyfile *.tgz *~ */*~ *.log \
      *.out *.bak tags html/ latex/ lisp-linux-*/ core ${TARGET}${EXE}

clean:
	@echo Cleaning repository.
	@echo $(foreach DEL,${CLEAN},$(shell ${RM} ${RM_FLAGS} ${DEL}))

##############################################################################

