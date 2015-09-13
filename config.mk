# "dwm" from <http://suckless.org/> inspired config file

# Version control variables and information

## These commands will depend on what version control is being run, or
## if any is being used at all. Currently git is being used.
VERSION    = $(shell git describe) 
VCS_COMMIT = $(shell git rev-parse --verify HEAD)
VCS_ORIGIN = $(shell git config --get remote.origin.url)

# Install paths

DESTDIR   ?= 
PREFIX 	  ?= /usr/local
MANPREFIX ?= ${PREFIX}/share/man

# Compiler and compiler flags

CC      ?= gcc
# The CFLAGS_RELAXED is used to compile main.c, main.c uses
# several libraries that require a cast from "void*" to a
# function pointer, which causes warnings which are unnecessary.
CFLAGS_RELAXED ?= -Wall -Wextra -g -fwrapv -std=c99 -O2 -fPIC
CFLAGS 	= $(CFLAGS_RELAXED) -pedantic
# Compilation options

## CPP defines:
### NDEBUG       Disable asserts
### USE_LINE     Add line editing capability, requires libline
### USE_DL	 Add support for dlopen, requires "-ldl".
DEFINES ?= -DUSE_LINE -DUSE_DL 
LINK    ?= -ldl
#LINK    ?= 
