## todo.md

A to-do list and scratch pad for this lisp implementation.

### Priority To-Do section

This is the priority to-do list, in order, these things are holding back the
future development of the project.

1. Rewrite internals to use cons cells.

The internals as of 2014-09-17 are based around resizeable arrays which is both
not very efficient and does not reflect the structure of how the lisp
interpreter should be. This is the number one priority and in doing this a lot
of code can be changed to be more elegant, for example instead of a nil object,
NULL can replace it in the cons cell. It makes loops operating over the cons
lists easier to program.

2. Tail Call optimization

Tail call optimization is a must, it is pretty much expected even, it can
implemented quite simply by putting more of the interpreter into **eval**;

<https://stackoverflow.com/questions/310974/what-is-tail-call-optimization>
<https://en.wikipedia.org/wiki/Tail_call>

3. Macros

A way of extending lisp from within lisp, proper macros, would be a goal. It
would not be complete without it. This topic will need to be researched
beforehand.

4. Bignums

Arbitrary precision arithmetic is a must, with rational types instead of floats
serving for all mathematical operations.

### Bugs

1. Garbage collection

I am not sure how effective my garbage collector is, it might not be working
correctly. To test it I should run the interpreter with Valgrind and "massif" to
get an overview what is getting freed when, initial testing did not prove to go
very well. 

2. Funarg problem and evaluation.

The funary problem is not solved correctly leading to incorrect code! Also
somethings are not evaluated at the right time.

### A more general To-Do section.

This is a more general to-do section, it might as well be treated as a "this
might be nice to have section" and it might duplicate that priority section in
its items.

* Orient the lisp towards processing text à la mode de awk/sed/tr 
  - Perhaps these could be implemented as a macro package?
* String interpolation like in many shell and scripting languages could
  be useful. How it would work and interact with lisp is another thing.
* Rewrite basic types used in implementation from arrays to cons
  cells as it should be.
 
* The parser should:
  - Handle arbitrary precision numbers
  - Handle arbitrary length strings
  - Parse special forms into enums such as
  "if", "begin", ...
  but do so *optionally*

* Write specifications for each of the modules. This along
  with Unit tests would *help* in rooting out bugs and *help*
  making the project more formal. Some modules should be easier
  than others like the "regex" modules, "eval" within the lisp
  interpreter and the "bignum" module (when complete).

* Change the directory structure.
  - Once the regex and hash libraries have been included into the
  project their directories can be removed and the test programs
  moved to the test bench suite
  - There should be a 'source' and an 'include' directory.
  - Libraries should be created, both static and dynamics ones.

* The core library is portable, however an external wrapper need
  not be and as such could implement:
  - dlopen wrapper to load dynamic library plug-ins
  - line editor library 
  - socket library (plug-in)
  - OpenGL interface (plug in)

* Make the API simpler to use;
  - Functions to set up internals
  - Do not allow stderr redirect? 
  ie. 
  io\_putc(char c, io * o, io * e);
  becomes
  io\_putc(char c, io * o);

* Rethink special forms;
  - Use "cond" instead of (or in addition to) "if".
  - Add "loop"

* Tail call optimization;
  This is a must, tail call optimization can be achieved by moving some
  primitives into eval

* I should be able to implement a metacircular interpreter like the
  one from <https://sep.yimg.com/ty/cdn/paulgraham/jmc.lisp>.

* NULL as nil
  Instead of a nil type and NULL being different, they should be the
  same, so there should not be a "nil" type, but instead it should be
  implicit from the fact that in the cons cell, one of the objects
  is NULL.

* Macros
  At the moment a macro system has not been implemented or designed

* Lisp bug; ./lisp -e 'ErroneousInput$48w9'
  This should fail

* Memcpy should check its arguments, as well as other library calls
  - Check realloc, string library functions, etc.

* Special parsing of if,begin,... etc. Best way to deal with this?

* Support Unicode
  Unicode is not supported and I do not know of the best way to support
  it yet.

* Move primops to a separate file

* Unit Tests / Test benches
  Writing unit tests / test benches would really help in moving the code
  to use different data structures, it would help in incrementally moving
  it from one to the other.

* Clean up code;
  - 0 for error
  - 1 for ok
  - if bool, not the current way

* I *really* should convert the program to use cons cells instead
  of arrays at its basic type to speed things up, as its just inefficient
  otherwise.
* Handle EOF on output
* Check everything for *consistency*
* Implement all basic primitive ops
* Put all primops in a separate C-file
* Macros with return in them should be removed

### lisp.c specific

* Check for return values on all functions that can fail!
* Better error handling; a new primitive type should be made
  for it, one that can be caught.
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
  - modules; keywords for helping in the creation of modules
  and importing them.

I should implement the following functions;

```c
        static expr primop_nummore(expr args, lisp l);
        static expr primop_nummoreeq(expr args, lisp l);
        static expr primop_numless(expr args, lisp l);
        static expr primop_numlesseq(expr args, lisp l);
        static expr primop_min(expr args, lisp l); 
        static expr primop_max(expr args, lisp l);
        static expr primop_lisp_eval(expr args, lisp l); /* eval a bunch of
                                                            strings*/
        static expr primop_read(expr args, lisp l);
        static expr primop_getc(expr args, lisp l);
        static expr primop_gets(expr args, lisp l);
        static expr primop_putc(expr args, lisp l);
        static expr primop_find(expr args, lisp l);     /* find a variable */
        static expr primop_regex(expr args, lisp l);    /* search for regex in
                                                           string*/
        static expr primop_remove(expr args, lisp l);   /* file remove */
        static expr primop_rename(expr args, lisp l);   /* file rename */
```

### Hash lib

* 'Delete' in hash implementation, rework the hash library
* Should use void\* (and sexpr\_t for the special case of this program)

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
* Small lisp interpreters:
  <http://www.sonoma.edu/users/l/luvisi/sl3.c>
  <http://www.sonoma.edu/users/l/luvisi/sl5.c>
* The Lambda Papers
  <http://library.readscheme.org/page1.html>
