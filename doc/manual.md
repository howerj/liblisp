## manual.md

This lisp interpreter is implemented in ANSI C with two exceptions; "stdint.h"
and it's associated fixed width types are used and "stdbool.h" *may* be used,
the boolean type primarily being used to convey meaning.

The interpreter is a work in progress and as such the language is not stable,
nor is the API. At the moment the source *is* the manual, something which will
only change once version 1.0 of the project is reached.

### The language

This lisp interpreter has several goals;

* Portability, which will be achieved by writing it in pure C.
* Be both an implementation and a library.
* It should be extensible, so OS dependent functionality can be added
  from outside of the core.
* Simple to use; At all levels it should be simple to use, either
  from an API point of view or from a language one.
* Processing text and shelling out commands to the operating system
  should be easy to do; inspiration is to be drawn from AWK and sed
  for this.
* Small; The implementation should be small, it should not exceed the
  arbitrary limit of 10,000 lines of code (excluding unit tests for
  the code, these can and should be as long and detailed as possible).

Efficiency is not a goal, the language should be used for scripting,
prototyping, small programs, glue logic and the like. It should also be
used as an embedded scripting language, as such, anything that is too
slow should just be written in C instead.

### Lisp API

Simplicity of the interface is of primary concern, to show case this an
implementation of a very basic interpreter is given;

```c
      #include "lisp.h"
      int main(void){ lisp_end(lisp_repl(lisp_init())); return 0;}
```

Which will read from standard in, print to standard out and direct any error
output to the standard error streams as expected.

