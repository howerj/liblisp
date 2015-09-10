# readme.md

This directory contains modules that can be loaded at run time, mostly
containing non portable code, for [liblisp][].

The module system is dependent on the [main.c][] example interpreter and the
associate functions and *global state*.

Currently the following modules are support:

        liblisp_sql.so : SQL module : An SQL interface using sqlite3

The following modules are planned:

        openGL or SDL module
        Tiny C Compiler module
        OS Module: For interfacing the operating system

[liblisp]: https://github.com/howerj/liblisp
[main.c]: main.c
