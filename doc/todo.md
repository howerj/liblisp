## todo.md

### A temporary to-do list:

* Unit tests for each module
* Orient the lisp towards processing text à la mode de awk/sed/tr 
* Rewrite basic types used in implementation from arrays to cons
  cells as it should be.
 
* The parser should:
  - Handle arbitrary precision numbers
  - Handle arbitrary length strings
  - Parse special forms into enums such as
        "if", "begin", ...
    but do so *optionally*

* Rethink special forms;
  - Use "cond" instead of "if"
  - Add "loop"

* Memory: Garbage collection,
  Garbage collection should act inside the lisp environment not globally.

* Tail call optimization;
  This is a must, tail call optimization can be achieved by moving some
  primitives into eval

* Macros
  At the moment a macro system has not been implemented or designed

* Lisp bug; ./lisp -e 'ErroneousInput$48w9'
  This should fail

* Memcpy should check its arguments, as well as other library calls
  - Check realloc, string library functions, etc.

* Special parsing of if,begin,... etc. Best way to deal with this?


* Move primops to a separate file

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

### lisp.c specific

* Check for return values on all functions that can fail!
* Better error handling; a new primitive type should be made
* for it, one that can be caught.
* Make the special forms less special!
* Make more primitives and mechanisms for handling things:
  - Register internal functions as lisp primitives.
  - time, perhaps; random can be acquired from /dev/urandom
  - eq > < <= >=
  - string manipulation and regexes; tr, sed, //m, pack, unpack, 
  split, join
  - type? <- returns type of expr
  - type coercion and casting
  - file manipulation and i/o: read, format, 
    read-char read-line write-string, ...
  - max, min, abs, ...
  - Error handling and recovery
  not, and, or, logical functions as well!
  - set related functions; intersection, union, member, ...
  - Memory control functions:
  - Force mark/collect
  - comment; instead of normal comments, comments and the
  unlisp\_evaluated sexpression could be stored for later retrilisp\_eval
  and inspection, keeping the source and the runnning program
  united.
  - modules; keywords for helping in the creation of modules
  and importing them.


### Notes

* assert("This is an assert comment" && (NULL != ptr));
* Use @bug in doxygen.
* Eventually I should be able to create a macro such that "ca+d+r" would parse
  correctly
* Add in references:
  <http://c2.com/cgi/wiki?ImplementingLisp>
  For example.
* Use Xmacros where appropriate:
  <https://en.wikipedia.org/wiki/X_Macro>
* Use Opaque Pointers where appropriate:
  <https://en.wikipedia.org/wiki/Opaque_pointer>

#### Hash lib

* 'Delete' in hash implementation, rework the hash library
* Should use void\* (and sexpr\_t for the special case of this program)
