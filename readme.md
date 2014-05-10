## readme.md

LSP Lisp:

Lisp Interpreter; an experimental (in the sense of personal learning) lisp
interpreter with minimal system dependencies to aide in portability.

It is currently still be developed and is in the **pre-alpha** phase.

The project is documented with [doxygen](http://www.stack.nl/~dimitri/doxygen/)
so you will not find much in the way of information in this "readme" file, 
but look to the code; start with [main.c](src/main.c) and [lisp.c](src/lisp.c),
I intend to make the documentation quite extensive!

* Author:
  - Richard James Howe
* License:
  - GPL v3.0
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

## Directory structure

* [bin/](bin/)
  * Contains the build after running *make*.

* [doc/](doc/)
  * Contains the doxygen configuration, the [manual](doc/manual.md) and
  any generated documentation files. Doxygen will generate its files
  under [doc/htm](doc/html)

* [lsp/](lsp/)
  * Contains any lisp programs.

* [src/](src/)
  * Contains all of the C sources files, that is, the actual interpreter.

