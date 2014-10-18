## readme.md

LSP Lisp:

Lisp Interpreter; an experimental lisp interpreter whose **core** only
depends on the C library being present. There maybe extensions to the
interpreter however that need other external libraries.

It is programmed in ANSI C, barring 'stdint.h' and 'stdbool.h', which
could be replaced if necessary with strictly ANSI C types. It compiles
with both the 'ansi' and 'pedantic' flags under GCC and Clang regardless.

It is currently still be developed and is in the **pre-alpha** phase.

The project is documented with [doxygen](http://www.stack.nl/~dimitri/doxygen/)
so you will not find much in the way of information in this "readme" file, 
but look to the code; start with [main.c](src/main.c) and [lisp.c](src/lisp.c),
I intend to make the documentation quite extensive! Eventually!

* Author:
  - Richard James Howe
* License:
  - LGPL v2.1 or later version
* Email:
  - <howe.r.j.89@gmail.com>

## Building and running

To make the project type:

```bash
    make
```

At the top level directory.

To run the lisp interpreter either type

```bash
  make run
```

Or change directories to build folder [bin/](bin/) and run

```bash
  ./lisp
```

If for any reason make fails to work change the directory
to the source directory [src/](src/) and run:

```bash
   cc *.c -o lisp
```

And that should create the interpreter in the same folder.

## Directory structure

* [bin/](bin/)
  * Contains the built executable and object files after running *make*.

* [doc/](doc/)
  * Contains the doxygen configuration, the [manual](doc/manual.md) and
  any generated documentation files. Doxygen will generate its files
  under [doc/htm](doc/html). Logs are also written to [doc/log](doc/log/).
  * There is also two sets of manual pages here, one for the interpreter
  API and one for the Lisp interpreter program.

* [lsp/](lsp/)
  * Contains any lisp programs.

* [src/](src/)
  * Contains all of the C sources files, that is, the actual interpreter.

## BUGS

* Yes

