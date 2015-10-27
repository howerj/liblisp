# liblisp configuration and build system options
# @todo Replace this with a ./configure script (possibly)
# @todo Centralize all operating system variables here
# @todo Clean up these build scripts, they are starting to
#       become a mess. Unlike C, or another programming
#       language, it is unclear what is acceptable or not.
# One way to clean up the Unix/Windows file separator problem:
#  <https://stackoverflow.com/questions/4058840/makefile-that-distincts-between-windows-and-unix-like-systems>
# With rules like this:
# ######################################
#   ifdef SystemRoot
#      RM = del /Q
#      FixPath = $(subst /,\,$1)
#   else
#      ifeq ($(shell uname), Linux)
#         RM = rm -f
#         FixPath = $1
#      endif
#   endif
#   
#   clean:
#       $(RM) $(call FixPath,objs/*)
#
# ######################################
# Which is a lot cleaner
#

# make run options
RUN_FLAGS=-Epc

# Version control variables and information
## These commands will depend on what version control is being run, or
## if any is being used at all. Currently git is being used.
VERSION    =$(shell git describe) 
VCS_COMMIT =$(shell git rev-parse --verify HEAD)
VCS_ORIGIN =$(shell git config --get remote.origin.url)

CC       ?= gcc
# The CFLAGS_RELAXED is used to compile main.c, main.c uses
# several libraries that require a cast from "void*" to a
# function pointer, which causes warnings which are unnecessary.
CFLAGS_RELAXED = -Wall -Wextra -g -fwrapv -O2 -Wmissing-prototypes
CFLAGS 	= $(CFLAGS_RELAXED) -pedantic

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
RPATH   = -Wl,-rpath=. -Wl,-rpath=./mod 

# And the rest of the OS dependent nonsense
## @warning This is not done in a declarative style, it 
##           might end up being wrong because of this.

# Install paths
FS        =/
DESTDIR   = 
PREFIX 	  = $(FS)usr$(FS)local
MANPREFIX = $(PREFIX)$(FS)share$(FS)man
# commands and their flags
AR       ?= ar
AR_FLAGS ?= rcs
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

ifeq ($(OS),Windows_NT)
FS      =$(subst /,\,/)
DESTDIR = C:$(FS)
PREFIX  = lisp
MANPREFIX = $(PREFIX)$(FS)share$(FS)man
RM=del
RM_FLAGS= /Q
LDCONFIG=
CP=copy
CP_FLAGS=
MV=move
CHMOD=echo
MKDIR_FLAGS=
RUN_FLAGS=-Ep
DLL=dll
EXE=.exe

else # Unix assumed {only Linux has been tested}

ifeq ($(FAST),true)
CFLAGS += -DNDEBUG
else
CFLAGS += -D_FORTIFY_SOURCE=2 -fstack-protector 
endif

CFLAGS += -fPIC

endif

CFLAGS += -std=c99

