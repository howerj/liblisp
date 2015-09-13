# readme.md

This directory contains modules that can be loaded at run time, mostly
containing non portable code, for [liblisp][].

The module system is dependent on the [main.c][] example interpreter and the
associate functions and *global state*.

All of the libraries installed and linked against with these modules must be
compiled as shared libraries. This might have to be arranged by the package
maintainer, for example the TCC development package on Debian 7 only contains
the static version of the library but the makefile contains options for
compiling and installing a shared version of the library.

Currently the following modules are support:

        liblisp_sql.so : An SQL interface using sqlite3
        liblisp_tcc.so : Tiny C Compiler module
        liblisp_os.so  : OS Module, For interfacing the operating system
        liblisp_crc.so : CRC module

The following modules are planned:

        openGL or SDL module

[liblisp]: https://github.com/howerj/liblisp
[main.c]: main.c
