## todo.md

### A temporary to-do list:

* The parser should:
  - Handle arbitrary precision numbers
  - Handle arbitrary length strings
  - Parse special forms into enums such as
        "if", "begin", ...
    but do so *optionally*

* Memory: Garbage collection,
  Garbage collection should act inside the lisp environment not globally.

* Unit Tests / Test benches
  Writing unit tests / test benches would really help in moving the code
  to use different data structures, it would help in incrementally moving
  it from one to the other.

* Clean up code;
  - 0 for error
  - 1 for ok
  - if bool, not the current way

* API:
  - All names should begin with file name prepended to function
  or variable name.

* Version calculation:

Calculating the version number *could* be useful, I guess. The process should
be automatic, something like:

```sh
    md5sum *.c *.h | md5sum | more_processing > version/version.h
```

* I *really* should convert the program to use cons cells instead
  of arrays at its basic type to speed things up, as its just inefficient
  otherwise.
* Go through to-dos in program
* Handle EOF on output
* Check everything for *consistency*
* Implement all basic primitive ops
* Put all primops in a separate C-file
* Create a better interface
* Rename wput, wprint, etc. No 'w' prefix, come up with another one
  to avoid confusion with 'wide' chars.
* Macros with return in them should be removed
* Abide more by the linux kernel style guides at:
  - <https://www.kernel.org/doc/Documentation/CodingStyle>

* What would it take to implement pipes, lisp s-expression pipes, with the
  I/O system?
  - Write output string on one lisp thread
  - Read input string on another lisp thread, blocking.
