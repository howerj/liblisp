## temp.md

### A temporary to-do list:

* Clean up code;
  - 0 for error
  - 1 for ok
  - if bool, not the current way

* API:
  - All names should begin with file name prepended to function
  or variable name.

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
