# liblisp configuration and build system options

# make run options

RUN_FLAGS=-Epc

# Version control variables and information
## These commands will depend on what version control is being run, or
## if any is being used at all. Currently git is being used.
VERSION    = $(shell git describe) 
VCS_COMMIT = $(shell git rev-parse --verify HEAD)
VCS_ORIGIN = $(shell git config --get remote.origin.url)

# Install paths

DESTDIR   ?= 
PREFIX 	  ?= /usr/local
MANPREFIX ?= $(PREFIX)/share/man

# Compiler and compiler flags

AR       ?= ar
AR_FLAGS ?= rcs
CC       ?= gcc
# The CFLAGS_RELAXED is used to compile main.c, main.c uses
# several libraries that require a cast from "void*" to a
# function pointer, which causes warnings which are unnecessary.
CFLAGS_RELAXED ?= -Wall -Wextra -g -fwrapv -O2 -Wmissing-prototypes
CFLAGS 	= $(CFLAGS_RELAXED) -pedantic

# Compilation options

## CPP defines of use
#   NDEBUG       Disable asserts
#   USE_DL	 Add support for dlopen/LoadLibrary, requires "-ldl" 
#                on Unix systems
DEFINES ?= -DUSE_DL
LINK    ?= -ldl

# And the rest of the OS dependent nonsense

## This does not work for TCC, but does for Clang and GCC
TARGET_SYSTEM := $(subst -, ,$(shell $(CC) -dumpmachine))

RM    	 = rm
RM_FLAGS = -rf
CP       = cp
CP_FLAGS = -f
MV       = mv
CHMOD    = chmod
MKDIR    = mkdir
MKDIR_FLAGS= -p
SED      = sed
PRELOAD ?= LD_LIBRARY_PATH="`pwd`/mod:`pwd`"
LDCONFIG = ldconfig

ifeq (mingw32, $(TARGET_SYSTEM))
#ifeq (mingw32, $(TARGET_SYSTEM))
PRELOAD=
RM=del
RM_FLAGS= /Q
LDCONFIG=
CP=copy
CP_FLAGS=
MV=move
CHMOD=REM
RUN_FLAGS=-Ep
else # Unix assumed {only Linux has been tested}
CFLAGS += -fPIC
endif
CFLAGS += -std=c99

