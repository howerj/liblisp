VERSION = 0.3.10

# Paths

DESTDIR = 
PREFIX = /usr/local
MANPREFIX = ${PREFIX}/share/man

# Libraries

# Options

DEFINES = -DUSE_LINE 

# Compiler

CC = cc

# Compiler flags

CFLAGS=-Wall -Wextra -g -fwrapv -std=c99 -pedantic -O2

