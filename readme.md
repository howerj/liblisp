## readme.md

Lisp Interpreter; an experimental (in the sense of personal learning) lisp
interpreter with minimal system dependencies to aide in portability.

### plan

Create lisp interpreter in stages:

1. Make parse with I/O and memory libraries.
  * I/O system can perform arbitrary redirection
  * Detailed error reporting
  * Safe memory allocation and garbage collection
2. Test parsing. 
3. Experimental lisp development phase.
4. Eval.
5. Extend interpreter.
6. Extend interpreter in lisp.

Misc:
Vim syntax highlighting.

## TODO

* Improve makefile
* Getopts
* Eval
* dynamic strings
* Unit tests; tests for each module.

primitives:
* Register internal functions as lisp primitives.
* random!
* car,cdr,cons,listlen,reverse, ...
* + - * % / , 
* eq = > < <= >=
* string manipulation and regexes; tr, s, //m, pack, unpack
* type? <- returns type of expr
* file manipulation and i/o: read, format, read-char read-line write-string,
* max, min
* not, and, or
* "recurse" keyword; simple (tail) recursion 
