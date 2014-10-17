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

A library has been added but a few things need to be changed, it is terribly
inefficient on so many fronts -- but it's simple! This needs to be incorporated
and optimized for performance (mainly fewer allocations and frees).

5. Improve performance.

Some of the ways the system is designed will be inherently slow even if it was
easy to do that way; I/O and memory allocation spring to mind. I/O should not be
done *one byte at a time* where possible. There are far too many memory
allocations.

6. Suppress return value of library function definitions 
   This is in the running interpreter. It is just annoying seeing them defined 
   all the time.

### Bugs

1. Garbage collection

I am not sure how effective my garbage collector is, it might not be working
correctly. To test it I should run the interpreter with Valgrind and "massif" to
get an overview what is getting freed when, initial testing did not prove to go
very well. 

2. Funarg problem and evaluation.

The funarg problem is not solved correctly leading to incorrect code! Also
somethings are not evaluated at the right time.

### A more general To-Do section.

This is a more general to-do section, it might as well be treated as a "this
might be nice to have section" and it might duplicate that priority section in
its items.

* Orient the lisp towards processing text à la mode de awk/sed/tr 
  - Perhaps these could be implemented as a macro package?
 
* The parser should:
  - Handle arbitrary precision numbers
  - Handle arbitrary length strings

* Signal handling plan:
  - Set up signals handlers for each standard C signals that sets global value
  to signal number.
  - Register variables with signal handler names and value of that handler
  - Check if signal global variable is zero or not, if not zero, set to zero
  and lookup handler for signal. This should be done in Eval.
  - The signal handler can be an arbitrary lisp expression

* New printf/scanf for fixed width types and no floating points, for I/O
  library.

* Write specifications for each of the modules. 
  This along with Unit tests would *help* in rooting out bugs and *help*
  making the project more formal. Some modules should be easier
  than others like the "regex" modules, "eval" within the lisp
  interpreter and the "bignum" module (when complete).

* Rethink special forms;
  - Use "cond" instead of (or in addition to) "if".
  - Add "loop"

* Tail call optimization;
  This is a must, tail call optimization can be achieved by moving some
  primitives into eval

* I should be able to implement a metacircular interpreter like the
  one from <https://sep.yimg.com/ty/cdn/paulgraham/jmc.lisp>.

* Lisp bug; ./lisp -e 'Erroneous\_Input'
  This should fail not silently do nothing.

* I should check memcpy arguments, as well as other library calls
  - Check realloc, string library functions, etc.

* Special parsing of if,begin,... etc. Best way to deal with this?

* Makefiles; I should simplify the build process.
  - Make makefiles simpler.
  - Make them portable to other make utilities, not just GNU make.

* Support UTF-8
  UTF-8 is not supported and I do not know of the best way to support
  it yet.

* Better debugging information. Printing allocated/freed memory locations
  would not go amiss.

* Move primops to a separate file

* Unit Tests / Test benches
  Writing unit tests / test benches would really help in moving the code
  to use different data structures, it would help in incrementally moving
  it from one to the other.

* Handle EOF on output
* Check everything for *consistency*
* Implement all basic primitive ops
* Put all primops in a separate C-file
* Macros with return in them should be removed

* Check for return values on all functions that can fail!
* Better error handling; a new primitive type should be made
  for it, one that can be caught.
* Make the special forms less special!

### The Interpreter

While the core interpreter should be written in C, and therefore compile with 
a conforming compiler, the demonstration application can be made OS specific,
requiring for example a POSIX API to work.

Examples of libraries and APIs I could implement functionality from:
  - dlopen wrapper to load dynamic library plug-ins
  - line editor library 
  - socket library (plug-in)
  - OpenGL interface (plug in)

And other behavior:
  - Have a "home" directory and configuration file.
  - Install a standard lisp library somewhere.

### Hash lib

* 'delete' in hash implementation, rework the hash library
* Should use void\* (and sexpr\_t for the special case of this program)

### Notes

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
