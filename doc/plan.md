## plan.md

The first attempt at the lisp interpreter will be to just get one up and no
matter how inefficient. It will also help me to build up libraries that I can
reuse in a next attempt, explore ideas (proper tail recursion, garbage
collection, lisp, text processing) that will allow me to make better choices in
the next iteration.

The steps and sections I need to work on are:

### Text processing primitives and file I/O

I would like to make file I/O and text processing easy like they are in perl,
awk and sed.

### Relatively Efficient

By better choice of data structures and algorithms, as well as architectural
decisions, I can implement a more efficient interpreter. I should move from an
array based approached to linked lists to help with insertion / deletion and
processing of environments.

### Garbage collection

This needs implementing, even if it is leaky at first. 

### Proper tail end recursion

Implementing proper tail end recursion will be vital in order to get a semi
usable interpreter.

### Modules, nested definitions, more scoping and binding

Implement better scoping and binding rules and methods. Ways to contain code
in modules would also be good.

### Documentation

Everything should be documented, in an incredibly detailed way, with doxygen
and in manuals (once the language implementation has settled onto something
reasonable).

### Unit Tests

Units tests against the core and against the interpreter should be done, each
of the sub units such as memory, the I/O wrapper, the S-expression parser in C,
should be easy to isolate and stress test with small programs.

