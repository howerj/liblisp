liblisp.md 
==========

# A small and extensible lisp interpreter and library
## Table of contents

1.  [Introduction](#Introduction)
2.  [Design Goals](#Design goals)
3.  [Documentation](#Documentation)
4.  [Building](#Building)
5.  [Deviations from other lisps](#Deviations from other lisps)
6.  [BUGS](#BUGS)
7.  [To Do](#To Do)
8.  [Test programs](#Test programs)
9.  [Manual](#Manual)
10. [References](#References)

<div id='Introduction'/>
## Introduction

This is a lisp interpreter written largely in [ANSI C][], with some [c99][], it is a
non-standard variant of lisp.

<div id='Design goals'/>
## Design goals

A summary of the design goals:

* Simple and Small (< 5000 lines of code)
* Extensible
* No implicit state
* Used as a generic utility library

The following functionality would be nice to put in to the core interpreter;
Support for matrices and complex numbers (c99 \_Complex), UTF-8 support, text
processing (diff, grep, sed, à la perl), map and looping constructs
from scheme, doc-strings, optional parsing of strings and float, apply,
proper constant and a prolog engine.

But this is more of a wish list than anything. Complex number support has
been added in a branch of the [git][] repository. Another interesting
possibility would be to use a big memory pool and allocate objects from that,
pointers would be replaces with indexes into this table, it would make saving
the state of the interpreter and restoring it a lot easier, plus would allow
for more advanced memory management in a portable way.

There are a lot of things that need tidying up and rethinking about,
especially when it comes to the interface of the library and the naming of
functions. More comments are also in order in the code explaining what the code
is meant to achieve is also in need.

If you do not like the formatting use the "indent" program from:
<https://www.gnu.org/software/indent/>.

### Rationale

* Simple and Small (< 3000 lines of code)

The interpreter and the language should be simple to understand. It provides
more of a DIY lisp environment where most facilities are not provided but are
up to the users to implement.

* Extensible

The core language is very small, with few primitive operations, as such it
needs to be extensible. New primitive functions needed be added by library
users, code fragments evaluated and even new types need adding.

* No implicit state

It should be possible to run multiple instances of the interpreter in the same
address space, this requires no implicit or hidden state in the interpreter
design, which impedes multiple instances running at once. There is currently no
hidden and implicit state within the interpreter.

* Used as a generic utility library

The library includes routines that might be useful in other programs that it
could have chosen not to expose, mostly relating to I/O and strings.

<div id='Documentation'/>
## Documentation

The header [liblisp.h][] contains all the documentation (in doxygen format)
relating to the API. The rational for this being the API document is that there
is only one document to maintain, as opposed to it and a [liblisp.3][] to keep in
sync. The [liblisp.3][] that is present simply points the user to the [liblisp.h][]
header.

<div id='Building'/>
## Building

To build the interpreter a C compiler for a hosted platform is needed. It has
no other dependencies apart from the C library. A makefile is provided, but the
following commands should all work for their respective compilers:

        gcc   *.c -lm -o lisp
        tcc   *.c -lm -o lisp
        clang *.c -lm -o lisp

The preprocessor defines that add or change functionality are:

        USE_LINE  Add a line editor, must be linked with libline.a
        USE_TCC   Add the Tiny C Compiler for run time code generation
        USE_DL    Add the dynamic loader interface
        NDEBUG    The interpreter liberally uses assertions

Using the makefile instead (either "make" or "make all") will build the "lisp"
executable as well as the "liblisp.a" library. It will also build "liblisp.so"
The libraries will be built with debugging symbols.

The line editing library is limited to Unix systems which support a
[VT220][] terminal emulator (or are lucky enough to the actual terminal
hooked up!), which should be all of them. It may also work with POSIX
compatibility layers such as [Cygwin][], but this has not been tested. The
repository is set up a fork of the [linenoise][] library that is simply called
[libline][], this has limited support for ["Vi"][] like key bindings.

Color support can be optionally enabled on all output channels and is reliant
on [ANSI escape codes][] on it being displayed correctly, if it is not simply
do not enable color, it is only a minor feature.

<div id='Deviations from other lisps'/>
## Deviations from other lisps

* Case sensitivity

Case sensitivity is both easier to implement, more general and allows
information to be encoded in case, much like in language with acronyms.
If the implementation were to be case insensitive the question of what
character encoding and sets we use would come up, we might design a lisp which
is case insensitive with the Latin alphabet, but why not with the Cyrillic
alphabet also?

* Does not follow any standards

As this implementation does not follow any standards there are bound to be
general behaviors and corner cases that are out of line with other standards
implementation, or are combinations of behaviors from other standards. The
implementation is the standard for now, which is a downside.

* '#' as comments as well as ';'

Another major deviation is that comments are represented by '#' as well as ';'
so that source code can easily be run as a Unix script with no special syntax
in the usual manner.

<div id='BUGS'/>
## BUGS

* If you manage to get a SEGFAULT then there is a bug in the code somewhere. It
  should not be possible to make the interpreter seg-fault.
* See the "@todo" comments at the top of [liblisp.c][] to see a full list of
  bugs as well comments labeled with "@bug".
* The code is formatted in an odd way, but I like it, it is simple enough to
  change however by using GNU indent.
* Some line lengths in the C file exceed 80 characters.
* The regex implementation does not work when built under Windows with the
  64-bit version of the Tiny C Compiler. This is related to this bug
  in version 0.9.26 of the compiler.
  <https://lists.nongnu.org/archive/html/tinycc-devel/2015-06/msg00000.html>
  with a solution posted here:
  <https://lists.nongnu.org/archive/html/tinycc-devel/2015-06/msg00001.html>
  The current Windows build script [make.bat][] is dependent on "tcc".

<div id='To Do'/>
## To Do

There are several issues that need resolving with the interpreter.

#### liblisp.c

* Environment lookup should be split into top level hash and cons
list for efficiency.
* Linear probing should be used for the hash function instead of
chained hashing, experimental code has been added for a fully working
linear probing hash implementation, but the lisp interpreter is dependent
on the internal representation of the hash functionality.
* The "struct hack" could be applied strings and other types, as
well as the length field being encoded in the variable length
section of the object.
* Generic error handling for primitives before they are called would
cut down on the amount of code needed in the primitives subroutines.
* File operations on strings could be improved. An append mode should
be added as well. freopen and/or opening in append mode need to be
added as well.
* Needed primitives; apply, loop. 
* Input, output and error IO ports should be added, these are different
  from *stdin*, *stdout* and *stdout*.
* Anonymous recursion would be a good thing to have.
* The semantics of the default IO port to **print** and **format**
to be worked out when it comes to printing color and pretty printing.
* There is currently no decent way of handling binary data.
* A branch that makes use of arbitrary precision arithmetic
  (replacing floating point numbers *and* integers) could be made.

#### main.c

* Some level of auto-completion could be added from the libline library.
* Experiment with auto insertion of parenthesis to make the shell
easier to use, for example if an expression is missing right parens
the line editing callback could add them in, or if there are more
than one atoms on a line it could enclose them in parens and see if
that can be evaluated "+ 2 2" would become "(+ 2 2)". This would only
be part of the line editor.
* Porting libline to Windows

<div id='Test programs'/>
## Test programs

The simple C test program is as follows:

        #include <liblisp.h>
        int main(int argc, char **argv)
        {
                return main_lisp(argc, argv);
        }

This does not show case the API but gives you a usable REPL to play with.

Or in C++:

        #include <iostream>
        #include <liblisp.h>
        using namespace std;
        int main(int argc, char **argv)
        {
                cout << "(cpp-test)" << endl;
                return main_lisp(argc, argv);
        }

The executable can read from files, "stdin" or evaluating strings. There is a
manual page for this interpreter, unlike for the header.

<div id='Manual'/>
## Manual

This is the manual for the language defined by this library. It does not
include any initialization code (that is lisp code that has been defined in
lisp and not in C and then passed to the interpreter on start up), but only
that language provided by [liblisp.c][] and [main.c][].

### [Lisp][]

[Lisp][] is the second oldest language after [FORTRAN][] that still has
descendants still in use. Despite its age it is a very high level language
built on simple principles; it had features that are now finding their way
into newer programming languages such as the [lambda procedure][]. It is a
multi-paradigm programming language built around a few core concepts:

1. [S-Expressions][]
2. [Garbage Collection][]
3. A mostly functional style of programming.

And their are various minor attributes that could also be said about the
language as well.

The first point however is very important, in [Lisp][] code and data are
represented in the same way, by the [S-Expression][]. Naturally any
programming language is going to have primitives and procedures for handling
the data types it is built to work with; [MATLAB][] is good at handling matrices,
[AWK][] with its associative arrays and regular expressions is good with
strings, [Lisp][] is good at handling [S-Expressions][].

Given the statements that:

1. In [Lisp][] code and data have the same format, [S-Expressions][].
2. [Lisp][] is designed to process [S-Expressions][].

It follows that [Lisp][] must be good at, or at least able to; read in,
manipulate, generate and write out [Lisp][] programs with relative ease. It can
also evaluate these expressions at run time. To put it another way; it is
possible to write [Lisp][] programs that write [Lisp][] programs.

The second important point, [Garbage Collection][], means that the programmer
does not have to worry about how objects are allocated or cleaned up, but it
does come at a price. The collector used here is a simple stop-the-world
[mark and sweep][] collector, this may add pauses at seemingly random times
throughout the programs execution.

The third point mostly is the least important, but means that techniques such
as [recursion][] (with [tail call][] optimization in this implementation) are
preferred and functions with [side effects][] are discouraged.

Some of these points are more historically important, most new languages now
have garbage collection. Lisp also was one of the first languages to have a
[REPL][], again many new languages also have this feature. REPL stands for
Read-Evaluate-Print-Loop and this is what the interpreter does, it reads in a
lisp expression, evaluates it, prints the result and loops to the beginning to
read in a new expression. This gives instant feedback on whether an operation
worked or now.

### Style

Code examples will be presented as follows:

        # Comments start with a '#' and end when the line does here =>
        # The '>' represents the prompt at which we can interactively
        # type expressions
        >
        > (+ 2 2) # "(+ 2 2)" is an expression
        4         # This is the return value of the evaluated expression
                  # note the lack of the '>' prompt.

All expressions return values in lisp, which will then be printed out in the
REPL, this will not be shown however for all output

        > (+ 2 2) # we already know this will print out 4 from above
        > (+ 3 3)
        6 # we show the new and exciting result

The canonical [metasyntactic variables][] will be used in the text; "foo",
"bar" and "baz" represent arbitrary expressions or atoms, changing depending on
the context.

Runs of *car* and *cdr* will be abbreviated, although the functions that
implement them might not be defined, examples as follows;

        > caar
        (lambda (x) (car (car x)))
        > cdar
        (lambda (x) (cdr (car x)))
        > cadr
        (lambda (x) (car (cdr x)))
        > cddr
        (lambda (x) (cdr (cdr x)))
        > caaddar
        (lambda (x) (car (car (cdr (cdr (car x))))))

Reading from left to right, an "a" adds a *car* and a "d" adds a cdr to the
front of the function.

How these are defined and what they mean will be talked about in later
chapters, they will have to be defined by the user if only the base library
defined in [liblisp.c][] is used and can be done so in terms of *car* and
*cdr*.

### An introduction to the interpreter

* Command line interface
  - The line editor and line history
* The REPL
* Command line options
* Environment variables used
  - HOME
* C API
* Interpreter Internals
* User defined primitive functions or "subr"s.
* User defined types

### An introduction to the language

This is a short introduction to the language, other very good sources of
information about specific lisp implementations include:

1. <http://www.schemers.org/Documents/Standards/R5RS/>

This is one of the scheme standards, at roughly 50 pages it is very small but
it describes a useful, usable language. [Scheme][] is one of the two popular
dialects of lisp. Scheme aims at being small and elegant.

2. <https://mitpress.mit.edu/sicp/>

The Structure and Interpretation of Computer Programs. A well known
introductory book on Scheme.

3. <http://www.lispworks.com/documentation/HyperSpec/Front/index.htm>

[Common Lisp][] is the other major dialect of lisp, it is a huge language and
is the other major dialect of lisp.

4. <http://shrager.org/llisp/index.html>

There are many dialects of lisp, both historical and current, this is a defunct
dialect that has a nice tutorial.

#### Atoms

An atom is the simplest data structure available to the lisp environment, it is
a primitive data structure; lists are composed of collections of atoms and
other lists and an atom cannot further be divided into smaller lists or atoms.

Atoms usually evaluate to themselves, apart from symbols which evaluate to
another atom or expression depending on the environment.

For example:

        > 2.0             # Floating point numbers are self evaluating
        2.0
        > 2               # Integer numbers are self evaluating
        2
        > +
        <SUBR:4231056>    # Evaluates to an internal subroutine
        > '+
        (quote +)         # quote postpones evaluation
        > "Hello, World!" # strings are self evaluating
        "Hello, World!"

Atoms can have different types, such as a hash, an IO port, a number, a string,
and a floating point number. They will be colorized differently depending on
what they are.

#### CONS cells

The [cons cell][] is the basic data structure from which lists and trees, and
so most structured data in this lisp apart from [associative arrays][], are
built. The *cons* subroutine is used to allocate and build new cons cells and
lists.

A cons cell consists of two cells, each of which can hold either a pointer to
other cons cells or data. Each cell will contain a few bits to determine what
the data is, or whether it is a pointer, as well as for other purposes such as
garbage collection.

The proper list:

        (1 2 3)

Looks like this in memory:

          CAR   CDR
        .-----.-----.     .-----.-----.     .-----.-----.
        |  1  | PTR |---->|  2  | PTR |---->|  3  | NIL |
        .-----.-----.     .-----.-----.     .-----.-----.

The first cell in the pair is called the "car" cell and the second the "cdr"
cell for historical reasons. "PTR" represents a pointer to another cell and
"NIL" represents the "nil" symbol, which is used to terminate a proper list.

Lists which are not terminated by a "nil" can also be made, they are called
[dotted pairs][].

The [dotted pair][]:

        (1 . 2)

Looks like this:

        .-----.-----.
        |  1  |  2  |
        .-----.-----.

Lists that end with a [dotted pair][] can also be made:

        (1 2 . 3)

Looks like this:

        .-----.-----.     .-----.-----.
        |  1  | PTR |---->|  2  |  3  |
        .-----.-----.     .-----.-----.


[Trees][] can also be made, the tree:

        (1 (2 3) 4)


Looks like this:

                          .-----.-----.     .-----.-----.
                          |  2  | PTR |---->|  3  | NIL |
                          .-----.-----.     .-----.-----.
                             ^
                             |
        .-----.-----.     .-----.-----.     .-----.-----.
        |  1  | PTR |---->| PTR | PTR |---->|  4  | NIL |
        .-----.-----.     .-----.-----.     .-----.-----.

Arbitrary lists and trees can be constructed.

#### List Manipulation

The lists shown above can be manipulated and created with built in functions.
To create new cons cells and list, *cons* is used, it takes to arguments and
returns a new list.

        > (cons 1 2)
        (1 . 2)
        > (cons 1 (cons 2 3))
        (1 2 . 3)
        > (cons 1 (cons 2 (cons 3 nil)))
        (1 2 3)
        > (cons (cons 1 2) (cons 3 nil))
        ((1 . 2) 3)

The cells in a cons pair can be accessed with the *car* and *cdr* functions,
the *car* function gets the first element in the pair, the *cdr* function gets
the second cell.

        > (car (cons 1 2))
        1
        > (cdr (cons 1 2))
        2
        > (car (cons 1 (cons 2 3)))
        1
        > (cdr (cons 1 (cons 2 3)))
        (2 . 3)
        > (car (cons 1 (cons 2 (cons 3 nil))))
        1
        > (cdr (cons 1 (cons 2 (cons 3 nil))))
        (2 3)
        > (car (cons (cons 1 2) (cons 3 nil)))
        (1 . 2)
        > (cdr (cons (cons 1 2) (cons 3 nil)))
        3

#### Evaluation and Evaluation of Special Forms

The evaluation of lisp expressions is very simple, but there are some
exceptions known as special forms. The standard evaluation rule is quite
simple, any variable in the first position of a list is treated as a function
to apply, the arguments to the function are first evaluated recursively then
feed into the function.

#### Function and variable definition

New functions and variables can be defined and they can be defined in global or
a lexical scope.

#### Control structures and recursion
#### Association lists

* Normal association Lists
* Extended association list (with hashes)

#### Fexprs

An [fexpr][] is a function, user defined or an internal, whose arguments are
passed into it unevaluated, allowing them to be evaluated or many times or not
at all within the [fexpr][]. They have fallen out of favor within lisps as they
defy code optimization techniques which is not a concern in this implementation
as none are performed (beyond optimizing [tail calls][]).

#### Example code

Example code and be found in the repository the library is hosted in;

* [init.lsp][]

This contains initialization code to the make the lisp interpreter much more
friendly, usable even. It contains code that acts as wrappers around
primitives, functions for manipulating [sets][], basic mathematical functions
such as [factorial][] and [gcd][], and functions for operating on lists.

* [meta.lsp][]

This contains a [Meta-circular Evaluator][], as defined in
[The Roots of Lisp][].

* [test.lsp][]

This contains a test suite, with limited coverage, for [init.lsp][]. It should
be run after [init.lsp][] is, it will exit the interpreter if any of the tests
fail.

### Debugging

The implementation has a few methods for debugging the code, beyond the power
of the REPL.

To view the definition of an expression we can type it at the prompt:

        # define a function to calculate the
        # Greatest Common Denominator
        > (define gcd (lambda (x y) (if (= 0 y) x (gcd y (% x y)))))
        (lambda (x y) (if (= 0 y) x (gcd y (% x y))))
        > gcd
        (lambda (x y) (if (= 0 y) x (gcd y (% x y))))

Unfortunately this does not apply to built in functions, it would be nice to
print out the C functions that compose the language primitives but it would be
too much work for too little reward, instead an indication that the object is
somehow *special* is printed out:

        > (+ 2 2)
        > +             # What is the definition of '+'?
        <SUBR:4228480>  # Unfortunately the C code would have to
                        # be consulted "<SUBR:" indicates that this
                        # is a built in language primitive the "4228480"
                        # is the pointer at which the function lives in
                        # the lisp executable.

Subroutines and "IO" types are unprintable types, they have no sensible
representation that could be printed out to the screen so are instead
shown as some detail about the object and a pointer to where it lives (that
means nothing to the user of the REPL apart to distinguish between two specific
objects). Effort has been made to reduce the number of types that cannot be
printed to a minimum.

User defined types may have been provided with a function to print them out, if
so it is used, if not then:

        <USER:pointer>

is printed, where *pointer* is an internal pointer to that object. "IO" types
are similar:

        > (open *file-out* "test.log") # open up a file for writing
        <IO:OUT:37908656>              # returned unprintable IO type, output
        > (open *file-in* "test.log")  # open up a file for reading
        <IO:IN:37909776>               # returned unprintable IO type, input

As much useful information about the object will be printed.

#### Tracing

Tracing objects as they are evaluated is off by default. It can be turned on
with the *trace-level!* primitive. There are currently three different trace
levels defined; off (the default), trace only symbols that have been marked for
tracing and trace everything.

        > (trace 'foo)      # is the symbol 'foo' marked for tracing?
        > (trace 'foo t)    # mark the symbol 'foo' for tracing
        > (trace 'foo nil)  # clear the symbol 'foo' of tracing mark

        > (trace foo)       # is the evaluated expression 'foo' marked for tracing?
        > (trace foo t)     # mark the evaluated expression 'foo' for tracing
        > (trace foo nil)   # clear the evaluated expression 'foo' of tracing mark

        > (trace-level! *trace-off*)    # turn tracing off
        > (trace-level! *trace-marked*) # turn on tracing for marked objects
        > (trace-level! *trace-all*)    # trace all objects, marked or not

Everything can be marked for tracing but doing so does not necessarily produce
anything useful. The tracing used is very simple and is to get a rough idea of
the control flow when it is applied to specific objects. Tracing everything
produces much more verbose output.

As an example:

        # A naive factorial implementation
        > (define factorial
         (lambda
          (x)
           (if
            (< x 2) 1
            (* x
             (factorial
             (- x 1)))))

        # turn full tracing on
        > (trace-level! *trace-all*)

        > (factorial 3)
        (trace
         (factorial 3))
        (trace factorial)
        (trace 3)
        (trace (begin (if (< x 2) 1 (* x (factorial (- x 1))))))
        (trace (if (< x 2) 1 (* x (factorial (- x 1)))))
        (trace (< x 2))
        # ... lot of trace output ...
        (trace (< x 2))
        (trace <)
        (trace x)
        (trace 2)
        (trace 1) # tracing ends here
        6 # The result of (factorial 3)

Whereas with tracing of specific symbols we can tell when they are called:

        > (trace-level! *trace-marked*)
        > (trace '* t)
        > (trace 'factorial t)
        > (trace 'x t)
        > (factorial 3)
        # The complete trace output
        (trace factorial)
        (trace x)
        (trace *)
        (trace x)
        (trace factorial)
        (trace x)
        (trace x)
        (trace *)
        (trace x)
        (trace factorial)
        (trace x)
        (trace x)
        6 # The result

It should not be expected that the trace output is the same as the above,
functionality may be arbitrarily added or removed as it is purely for use as a
debugging aide, one that is bound to be improved upon.

#### Error messages

The error messages printed are quite succinct, although to be fully useful the
C source would have to be referred to and decoded.

        > (+ 2 2)
        4
        > (+ 2)
        (error 'subr_sum "argument count not equal 2" '(2)" "liblisp.c" 1337)
        error # The returned value. (error ... ) is the error message printed.
        # Calling open with arguments around
        > (open "file.log" *file-in*)
        (error 'subr_open "expected (integer string)" '("file.log" 1) "liblisp.c" 1600)
        error

The error message format is *usually* as follows:

        (error PRIM MSG ARGS C-FILE C-LINE)

        PRIM:   The C-level function the error occurred in.
        MSG:    An arbitrary, but hopefully useful message,
                often containing information on the number
                and type of arguments expected
        ARGS:   The actual arguments passed into our function.
        C-FILE: The C-file the error occurred in.
        C-LINE: The line of the C-file the error occurred in.

The format of what occurs in an error message should not be counted on, it is
purely for the users benefit.

### List of primitives, special forms and predefined symbols

Here follows a complete list of all symbols and primitives used within the
library and their behavior.

#### Special forms and variable

        nil                Nil, the object representing false
        t                  True, the object representing true

        quote              Return an unevaluted expression
        if                 Conditional evaluation
        lambda             Create a lambda procedure
        flambda            Create an "fexpr"
        define             Define a new variable with a value
        set!               Change an already defined variables value
        begin              Evaluate a sequence of S-Expression, return the last
        cond               Conditional evaluation
        environment        Return a list of all in scope variables
        let*               Create a new variable scope
        letrec             Create a new variable scope, allowing recursion
        error              Error, an object representing an error

##### Special variables

There are several special variables used within the interpreter.

* nil

        > nil
        ()
        > ()
        ()
        > (cdr '(a))
        ()

Represents false and the end of a list. Can also be written as '()'.

* t

        > t
        t

Represents true, evaluates to itself.

##### Special forms

* quote

Quote returns an unevaluated expressions so expressions can be programmatically
manipulated.

        (quote EXPR)
        'EXPR
        # Examples:
        # Expression to evaluate.
        > (+ 2 2)
        4
        # Postpone evaluation of an expression.
        > (quote (+ 2 2)) # (+ 2 2) is not evaluated.
        (+ 2 2)           # Returns the unevaluated expression
        # '(+ 2 2) is equivalent to (quote (+ 2 2))
        > '(+ 2 2)
        (+ 2 2)

* if

The "if" special form expects three expressions, if the first evaluates to true
(which is any value apart from 'nil') then it returns the second expression
after evaluation. If that test evaluated to false (that is to 'nil') then it
returns the third expression after evaluation. Only the test and the expression
to be returned are evaluated.

        (if TEST EXPR EXPR)
        # Examples:
        > (if t 1 2)
        1
        > (if nil 1 2)
        2
        > (if "hello" 1 2)
        1
        > (if 0 'a 'b) # 0 is not 'nil', it evaluates to true here!
        a # return 'a' because 0 is true in this context.

* lambda

Create a new [lambda procedure][] or anonymous function that can be applied,
assigned to a label and passed around like any other expression. The arguments
to the procedure are evaluated before they are passed in.

        (lambda (VARIABLE VARIABLE ...) EXPR)
        # Examples:
        > ((lambda (x) (* x x)) 4)
        16
        > (define square (lambda (x) (* x x)))
        (lambda (x) (* x x))
        > (square -5)
        25
        > (square (+ 2 3))
        25

* flambda

Create a new [fexpr][], this is similar to a [lambda procedure][] except all
the arguments are turned into a list and bound to a single variable. The
arguments are *not* evaluated when they are passed in.

        (flambda (VARIABLE) EXPR)
        # Examples:
        > (define print-me (flambda (x) x))
        (flambda (x) x)
        > (print-me hello 'world (+ 2 2))
        (hello (quote world) (+ 2 2))

This allows the creation of functions that evaluate can delay, not or
reevaluate their arguments.

* define

Adds a symbol value pair to the global namespace. It evaluates the argument
that will become the value first.

        (define EXPR EXPR)
        # Examples:
        > (define foo 3)
        3
        > foo
        3
        > (+ foo foo)
        6
        > (define bar (* 9 9)) # Note (* 9 9) is evaluated.
        81
        > bar
        81

* set!

Change the value of an already defined variable to another value.

        (set! EXPR EXPR)
        # Examples:
        > (define a 1)
        1
        > a
        1
        > (set! a 2)
        2
        > a
        2
        > (set b 3)
        (error 'eval 'set! "undefined variable" '(b 3) "liblisp.c" 1238)
        error

* begin

Sequence a list of expressions for evaluation; expressions are evaluated from
left to right and only the value of the last expression is returned.

        (begin)
        (begin EXPR ...)
        # Examples:
        > (begin)
        ()
        > (begin (+ 1 1) "hello" (+ 2 2))
        4

* cond

Given pairs of expressions it evaluates the first expression in the pair and if
it returns true it will return the second value in the pair after it is
evaluated. It will continue along the list until one of the first values in the
pairs evaluates to true, if none do it will return nil.

        (cond)
        (cond (EXPR EXPR) ...)
        # Examples:
        > (cond)
        ()
        > (cond (nil 1) (nil 2))
        ()
        > (cond (nil 1) (t 2) (nil 3))
        2
        > (cond (nil 1) ((if 'a t nil) 2) (t 3))
        2

* environment

Returns an association list of the current environment, the list is quite large
as it contains every defined symbol and what they evaluated to as well as all
the symbols defined in the current and encapsulating scopes. It returns an
association list.

        > (defined square (lambda (x) (* x x)))
        (lambda (x) (* x x))
        > (environment)
         ((square . (lambda (x) (* x x))) # Most recently defined top level
                                          # definition is the first entry
         ...
         (*seek-cur* . 1)
         (e . 2.718282)
         (pi . 3.141593)
         (t . t))
        > (let (x 4) (environment))
        ((x . 4)
         ...
         (*seek-cur* . 1)
         (e . 2.718282)
         (pi . 3.141593)
         (t . t))

* let\*

Creates new temporary local bindings for variables and evaluates expressions
with them. The format is:

        (let (SYMBOL VALUE) (SYMBOL VALUE) ... EXPR)

        # Examples:
        > (let* (x 3) (y (+ x x)) y)
        6
        > (define baz (lambda (y) (let* (foo -2) (bar 2) (+ (* foo y) bar))))
        > (baz 5)
        -8

* letrec

This is like let\*, but the SYMBOL is defined first, then the VALUE is
evaluated, which is then bound to the SYMBOL. The upshot of all that is now
recursive procedures can be given local scope.

        (let (SYMBOL VALUE) (SYMBOL VALUE) ... EXPR)

        # Examples:
        > (letrec
           (factorial (lambda
            (x)
            (if
             (< x 2) 1
             (* x
              (factorial
               (- x 1))))))
           (factorial 6))
        720
        > (define f
           (letrec
            (factorial (lambda
             (x)
             (if
              (< x 2) 1
              (* x
               (factorial
                (- x 1))))))
            factorial)
        > (f 6)
        720

* error

The error special form can be used to throw errors, it is also used as a
representation of an error returned from primitive functions.

        (error)
        (error INTEGER)

        > (error 1)
        error
        > (error -1) # The error message here does not make much sense.
        (error 'lisp_repl "invalid or incomplete line" "liblisp.c" 2265)


#### Built in subroutines

The following is a list of all the built in subroutines that are always
available.

* &

Perform the bitwise and of two integers.

        # (& INT INT)
        > (& 1 1)
        1
        > (& 3 1)
        1
        > (& 3 4)
        0


* |

Perform the bitwise or of two integers.

        # (| INT INT)
        > (| 0 1)
        1
        > (| 1 1)
        1
        > (| 2 1)
        3
        > (| 4 3)
        7
        > (| 2 1.0) # Does not work with floats.
        (error 'subr_bor "expected (int int)" '(2 1.000000) "liblisp.c" 1332)
        error

* ^

Perform the bitwise exclusive of two integers.

        # (^ INT INT)
        > (^ 1 1)
        0
        > (^ 3 1)
        2

* ~

Perform the bitwise inversion of an integer.

        # (~ INT)
        > (~ 0)
        -1
        > (~ -1)
        0
        > (~ 4)
        -5

* \+

Add two numbers together.

        # (+ ARITH ARITH)
        > (+ 2 2)
        4
        > (+ 3.2 2)
        5.2          # Converted to float
        > (+ 2 3.2)
        5            # Converted to integer
        > (+ -2 -1)
        -3

* \-

Subtract the second argument from the first.

        # (- ARITH ARITH)
        > (- 2 3)
        -1
        > (- 2 -3.0)
        5
        > (- 1.0 9)
        -10.0

* *

Multiply two numbers together.

        (* ARITH ARITH)

* %

Calculate the remainder of the first number divided by the second.

        (% INT INT)

* /

Calculate the first number divided by the second.

        (/ ARITH ARITH)

* =

Test for equality between two expressions. This is an alias for "eq".

        (= EXPR EXPR)

* eq

Test for equality between two expressions. This is the same function as "=".

        (eq EXPR EXPR)

* >

Test whether the first argument given is greater than the second argument.
This function works for floats, integers and strings.

        (> ARITH ARITH)
        (> STRING STRING)

* <

Test whether the first argument given is less than the second argument.
This function works for floats, integers and strings.

        (< ARITH ARITH)
        (< STRING STRING)

* cons

Create a new cons cell from two expressions.

        (cons EXPR EXPR)

* car

Get the first value from a CONS cell.

        (car CONS)

* cdr

Get the second value from a CONS cell.

        (cdr CONS)

* list

Create a list from a series of expressions.

        (list)
        (list EXPR ...)

* match

A very simple string matching routine. This routine takes a pattern as its
first argument and a string to match the pattern against as its second
argument.

        # (match STRING STRING)
        > (match "hell?" "hello")
        t

* scons

Concatenate two strings together.

        (scons STRING STRING)

* scar

Make a string from the first character of a string.

        (scar STRING)

* scdr

Make a string from a string excluding the first character.

        (scdr STRING)

* eval

Evaluate an expression in either the top level environment or with a specially
craft association list.

        (eval EXPR)
        (eval EXPR A-LIST)

* trace-level!

Set the level of "tracing" to be done in the environment, whether tracing is
off, on only for marked objects or on for all objects evaluated.

        (trace-level! ENUM)

* gc

Control garbage collection. If given no arguments this routine will force the
collection of garbage now, otherwise an enumeration can be passed in to change
when garbage is collected, if at all.

The garbage collector is on by default, or in the \*gc-on\* state. Garbage
collection can be postponed with the \*gc-postpone\* enumeration, in this state
the collector will not run but cells will still be added to the list of all
variables to collect, the collector can be turned on at a later date.

The collector can be turned off completely, *this will leak memory*, after
references are lost they are lost permanently. The collector cannot be turned
back on after this, the enumeration to do this is \*gc-off\*.


        (gc)
        (gc ENUM)

* length

Returns the length of a list or a string.

        (length LIST)
        (length STRING)

* input?

Returns 't' if the expression passed to it is an IO port set up for input,
'nil' otherwise.

        (input? EXPR)

* output?

Returns 't' if the expression passed to it is an IO port set up for output,
'nil' otherwise.

        (output? EXPR)

* eof?

Test whether an IO port has its End-Of-File marker set.

        (eof? IO)

* flush

Given no IO port it will flush all FILEs in the *program*, given an IO port it
will attempt to flush that specifically.

        (flush)
        (flush IO)

* tell

Return the current IO port position indicator.

        (tell IO)

* seek

Perform a file seek on an IO port. This can be relative to the beginning of the
file, from the current offset or from the end.

        (seek IO INT ENUM)

* open

Open up an IO port for reading or writing. The string can either be the name of
a file to be read in, or a string that can be opened as an IO port for reading.

        (open ENUM STRING)

* get-char

Read in a character, if no input port is given it will use the standard IO
input port. It returns a integer representing that character.

        (get-char)
        (get-char IN)

* get-delim

Read in a record delimited by a character, uses the default input port if none
is given. A string can be provided, but only the first character of the string
is used at the moment as a delimiter. An integer can be provided representing
that character instead, this can included the End-Of-File marker, which allows
you to slurp an entire file into one string. If the record is not delimited by
that character it will read the entire contents until the end of the file is
encountered.

If no input port is provided it will take its input from the standard input
port.

        # (get-delim STRING)
        # (get-delim INTEGER)
        # (get-delim IN STRING)
        # (get-delim IN INTEGER)

Given a file containing the following text:

        'My name is Ozymandias, king of kings:
        Look on my works, ye Mighty, and despair!'

Called "ozy.txt":

        > (define ozy (open *file-in* "ozy.txt"))
        <IO:IN:8258784>
        > (get-delim *eof*)
        "'My name is Ozymandias, king of kings:\nLook on my works, ye Mighty, and despair!'\n"
        > (seek ozy *seek-set* 0)
        0
        > (get-delim "\n")
        "'My name is Ozymandias, king of kings:"
        > (get-delim "\n")
        "Look on my works, ye Mighty, and despair!'"
        > (get-delim "\n")
        ()
        > (seek ozy *seek-set* 0)
        0
        > (get-delim ",")
        "'My name is Ozymandias"
        ...

* read

Read in an S-Expression, if no input port is given it will use the standard IO
input port. This returns an S-Expression, not a string. A string can be read as
well.

        # (read)
        # (read IN)
        # (read STRING)
        > (read "(+ 2 2)")
        (+ 2 2)

* put

Write a string to an IO output port, if no IO port is given it will use the
standard output IO port (which may be different from stdout).

        # (put STRING)
        # (put OUT STRING)

        > (put "Hello\tWorld\n") # Print a string to the standard output port
        Hello   World            # String written is printed out in raw form
        "Hello\tWorld\n"         # String written is returned in escaped form
        > (define a-file (open *file-out* "a.txt"))
        <IO:OUT:33884688>
        > (put "Hello\tWorld\n")
        "Hello\tWorld\n"         # String written is returned in escaped form
        # The file "a.txt" should now contain the string Hello   World in it.


* put-char

Write a character to an IO output port. If no output port is given it will
write to standard output IO port.

        (put-char INT)
        (put-char OUT INT)

* print

Print out an S-Expressions, to standard output IO port if no output port is
given.

        (print EXPR)
        (print OUT EXPR)

* ferror

Tests whether an IO port has its error flag set.

        (ferror IO)

* system

Given no arguments it will test whether the systems command processor is
available. Given a string it will attempt to execute a command with the systems
command processor, returning an integer for the return status of the command.

        # (system)
        # (system STRING)
        > (system)
        1               # indicates a command processor is avaiable
        > (system "ls") # Whether this works is implementation defined
        doxygen       exp       libline    liblisp.c    liblisp.md  lisp.1  make.bat  readme.md
        doxygen.conf  hist.lsp  liblisp.3  liblisp.h    liblisp.o   main.c  makefile  test.lsp
        doxygen.log   init.lsp  liblisp.a  liblisp.htm  lisp        main.o  meta.lsp
        0

* remove

Remove a file specified by a string or symbol passed to it.

        # (remove STRING)
        > (remove "file-that-does-not-exist.txt")
        nil
        > (remove "file-that-exists.txt")
        t

* rename

Rename a file, the first argument is the source, the second the destination.

        # (rename STRING STRING)
        > (rename "a.txt" "b.txt")
        t # given "a.txt" exists and no permission problems happen.
        > (rename "a.txt" "b.txt") # "a.txt" no longer exists, it cannot be
                                   # renamed
        nil
        > (rename "b.txt" "a.txt")
        t

* hash-create

Create a new list from a series of key-value pairs. The number of arguments to
the function must be even (which includes zero).

        # (hash-create)
        # (hash-create {STRING EXPR}... )
        > (hash-create)
        (hash-create)
        > (hash-create 'key1 'val1)
        (hash-create "key1" 'val1)  # Not that this is not a list altough it
                                    # looks like one
        > (hash-create 'key1 'val1 "key2" '(arbitrary list))
        (hash-create "key1" 'val1 "key2" '(arbitrary list))

* hash-lookup

Lookup a string in a hash.

        # (hash-lookup HASH STRING)
        > (define foo (hash-create 'key1 'val1 "key2" '(arbitrary list)))
        (hash-create "key1" 'val1 "key2" '(arbitrary list))
        > (hash-lookup foo 'key1)
        (key1 . val1)
        > (hash-lookup foo 'key2)
        (key2 . (arbitrary list))
        > (hash-lookup foo 'key3)
        ()
        > (hash-lookup foo "key1")
        (key1 . val1)

* hash-insert

Insert a key-value pair into a hash table.

        # (hash-insert HASH SYMBOL EXPR)
        > (define foo (hash-create 'key1 'val1 "key2" '(arbitrary list)))
        (hash-create "key1" 'val1 "key2" '(arbitrary list))
        > (hash-insert foo 'hello "world")
        (hash-create "key1" 'val1 "key2" '(arbitrary list) "hello" '"world")
        > (hash-lookup foo "hello")
        (hello . "world")

* coerce

Coerce one type into another, if the coercion is possible, if not it throws an
error. All objects can be coerced into the same type as the object, for example
hashes can be coerced into hashes, integers into integers, etc.

Some of the mappings have additional restrictions on them, such as when mapping
an Integer from a String, the string must represent a valid number such as;
"-1", "0", "99" and not "99a", "not-a-number" or even "1.0".

The mappings are:

* Integer from String, Float

All floats can be converted to integers.

All strings must represent a valid integer, containing only characters and
forms that would be valid integer sequences if feed into the interpreter raw.

This means all strings of the form:

        (+|-)?(0[xX][0-9a-fA-F]+|0[0-7]*|[1-9][0-9]+)

Can be coerced into integers.

* List from String, Hash

All strings and hashes can be coerced into lists.

* String from Integer, Symbol, Float

All integers, symbols and floats can be coerced into strings.

* Symbol from String

Most strings can be coerced into symbols, baring those strings with
white spaces, single or double quotes, parentheses or the black slash
characters.

* Hash from List

The list being coerced into a hash needs to have an even number of elements in
it, the even elements need to be either symbols or strings.

* Float from Integer, String

All integers can be converted to floats, the strings however need to represent
a valid floating pointing number.

        # (coerce ENUM EXPR)
        > (coerce *integer* 5.3)
        5
        > (coerce *hash* '(a 3 "b" (+ 2 2) c val))
        (hash-create "a" '3 "b" '(+ 2 2) "c" 'val)
        > (coerce *float* "-3.2e+4")
        -32000.000000
        > (coerce *symbol* "hello")
        hello

* time

Return a number representing the time defined by the implementation of the C
library that this program was linked against.

        # (time)
        > (time)
        1438547211
        > (time)
        1438547212

* getenv

Get an environment variable from the system based on a string.

        # (getenv STRING)
        > (getenv "SHELL")
        "/bin/bash"        # returned value is implementation defined
        > (getenv "TERM")
        "screen-256color"  # returned value is implementation defined

* random

Return a number from the Pseudo Random Number Generator.

        # (random)
        > (random)
        4451522206897829187
        > (random)
        3015435609869357244

* seed

Seed the random number generator.

        # (seed INT INT)
        > (seed 1 1)
        t
        > (seed 1 1.0)
        (error 'subr_seed "expected (integer integer)" (1 1.000000) "liblisp.c" 1965)
        error

* date

Return a list representing the date.

        # (date)
        > (date)
        (2015 8 2 20 18 47)
        > (date)
        (2015 8 2 20 18 50)

* assoc

Find an atom within an association list, an association list is a list
containing a series of dotted pairs, so:

        ((a . 1) (b . 2) (c . 3))

Is a valid association list. Association lists are used for to store and lookup
small numbers of key-value pairs. *assoc* returns the dotted pair in which the
*car* of it is equal to the *ATOM* supplied, it returns *nil* if cannot find
one.

        > (assoc ATOM A-LIST)
        > (define a-list '((a . 1) (b . 2) (c . 3)))
        ((a . 1) (b . 2) (c . 3))
        > (assoc 'a a-list)
        (a . 1)
        > (assoc 'c a-list)
        (c . 3)
        > (assoc 'd a-list)
        ()

* locale!

Set the locale string.

        (locale! ENUM STRING)

* trace

Check whether an expression has its trace mark set, or set the trace mark on an
expression on or off.

        (trace EXPR)
        (trace EXPR T-or-NIL)

* binary-logarithm

Calculate the binary logarithm of an integer.

        # (binary-logarithm INT)
        > (binary-logarithm 4)
        2
        > (binary-logarithm 99)
        6
        > (binary-logarithm 127)
        127
        > (binary-logarithm 128)
        7
        > (binary-logarithm -1)
        63 # implementation defined
        > (binary-logarithm 1.0)
        (error 'subr_binlog "expected (int)" '(1.000000) "liblisp.c" 1374)
        error

* close

Close an IO port.

        (close IO)

* type-of

Return an integer representing the type of an object.

        # (type-of EXPR)
        > (eq *integer* (type-of 1))
        t
        > (eq *integer* (type-of 1.0))
        ()
        > (type-of "Hello, World")
        6 # implementation defined

* timed-eval

Evaluate an expression in an environment, as with *eval*, but *cons* the time
taken to execute (in seconds) with the returned value from *eval*.

        # (timed-eval EXPR)
        # (timed-eval EXPR A-LIST)
        # In this example "monte-carlo-pi" is a function which attempts to
        # approximate the value of pi with the Monte Carlo Method, the value
        # passed to "monte-carlo-pi" is the number of iterations to perform,
        # the more performed the closer it should get to the value of pi.
        > (monte-carlo-pi 400)
        3.150000
        > (timed-eval '(monte-carlo-pi 400))
        (0.040000 . 3.280000)
        > (timed-eval '(monte-carlo-pi 4000))
        (0.430000 . 3.127000)
           ^           ^
           |           |
        Exec time    Approximation of "pi"

* reverse

Reverse a string, a list, or a hash (exchanging keys for values).

        # (reverse STRING)
        # (reverse LIST)
        # (reverse HASH)

* split

Split a string based on a regular expression.

        # (split STRING)
        # (split STRING STRING)

* join

Join a list of strings together with a separator between them.

        # (join STRING STRING...)
        # (join STRING (STRING...))

* substring

Create a substring from a string. There are two forms of this function, the
first can accept any integer, positive or negative (including zero), the second
form can accept two positive integers (including zero). 

        # (substring STRING INTEGER)
        # (substring STRING INTEGER INTEGER)

* format

Print out a list of expressions based on a format string. The format string is
the first argument, it is quite simple (for now).

        %%      print out a percent character '%'
        %s      print out a string, expects a string argument
        %c      print out a character, argument can either be an integer
                or string of length one
        %S      print out an expression

Any other character is printed out. If an argument is mismatched, there are too
few arguments, or there are too many, an error is thrown.

        # (format IO STRING EXPRS)
        # (format STRING EXPRS)
        > (format "examples: %S %s\n" '(a b (c d)) "a string!")
        examples: (a b (c d)) a string!
        "examples: (a b (c d)) a string!\n"
        > (format "%c %c\n" 104 "h")
        h h
        "h h\n"
        > (format "percent %%\n")
        percent %
        "percent %\n"

* regex-span

        # (regex-span STRING STRING)

* raise

Raise a signal, this function affects the entire program and can cause it to
halt.

        # (signal INTEGER)

* isalnum?

Returns 't if argument is alphanumeric, nil otherwise. This function can be used on
integers or strings.

        # (isalnum? INTEGER)
        # (isalnum? STRING)
        > (isalnum? "")
        ()
        > (isalnum? "abcABC123")
        t
        > (isalnum? 97) # True if using ASCII, 97 == ASCII 'a'
        t

* isalpha?

Returns 't if argument consists of alphabetic characters, nil otherwise. 
This function can be used on integers or strings.

        # (isalpha? INTEGER)
        # (isalpha? STRING)
        > (isalpha? "abcABC")
        t

* iscntrl?

Returns 't if argument consists of control characters, nil otherwise. 
This function can be used on integers or strings.

        # (iscntrl? INTEGER)
        # (iscntrl? STRING)


* isdigit?

Returns 't if argument consists of digits, nil otherwise. This function can be 
used on integers or strings.

        # (isdigit? INTEGER)
        # (isdigit? STRING)
        > (isdigit? "0123")
        t

* isgraph?

Returns 't if argument consists of printable characters (excluding space), 
nil otherwise. This function can be used on integers or strings.

        # (isgraph? INTEGER)
        # (isgraph? STRING)
        > (isgraph? "1aAB2.#")
        t

* islower?

Returns 't if argument consists of lower case characters, nil otherwise. 
This function can be used on integers or strings.

        # (islower? INTEGER)
        # (islower? STRING)
        > (islower? "abc")
        t

* isprint?

Returns 't if argument consists of printable characters, nil otherwise. 
This function can be used on integers or strings.

        # (isprint? INTEGER)
        # (isprint? STRING)
        > (isgraph? "1a AB2.#")
        t

* ispunct?

Returns 't if argument consists of punctuation characters, nil otherwise. 
This function can be used on integers or strings.

        # (ispunct? INTEGER)
        # (ispunct? STRING)


* isspace?

Returns 't if argument consists of whitespace, nil otherwise. This function 
can be used on integers or strings.

        # (isspace? INTEGER)
        # (isspace? STRING)


* isupper?

Returns 't if argument consists of upper case characters, nil otherwise. 
This function can be used on integers or strings.

        # (isupper? INTEGER)
        # (isupper? STRING)


* isxdigit?

Returns 't if argument consists of hexadecimal digits, nil otherwise. 
This function can be used on integers or strings.

        # (isxdigit? INTEGER)
        # (isxdigit? STRING)

* tr

Translate a string given a mode and two sets. This function is similar to the
"tr" program available in Unix environments and the C functions that back this
subroutine can be used to implement that program. The lisp interface is a bit
clunky, but it is a useful subroutine.

The mode string, the first argument, determines how the conversion should
proceed. Some of the arguments are mutually exclusive.

       ''  (empty string) normal translation
       'x' normal translation
       'c' compliment set 1
       's' squeeze repeated character run
       'd' delete characters (s2 should be NULL)
       't' truncate set 1 to the size of set 2

        # (tr STRING STRING STRING STRING)
        #       ^      ^      ^      ^
        #      mode  set-1  set-2  string
        > (tr "x" "abc" "def" "aaaabbaaaccc") 
        "ddddeedddfff"
        > (tr "t" "abc" "def" "aaaabbaaaccc") 
        "ddddeedddfff"
        > (tr "s" "abc" "def" "aaaabbaaaccc") 
        "dedf"
        > (tr "ts" "abc" "de" "aaaabbaaaccc") 
        "dedccc"

##### Additional functions

Functions added optionally in [main.c][]:

For all the mathematical functions imported from the C math library the
arguments are converted to floating point numbers before, the functions also
all return floating point values, apart from [modf][] which returns a cons of
two floating point values.

* log

Calculate the [natural logarithm][] of a floating point value. Integers are
converted to floating point values before hand.

        # (log ARITH)
        > (log e)
        1.0
        > (log 1)
        0.0
        > (log -1)
        nan
        > (log 10)
        2.302585
        > (log 3.3)
        1.193922

* log10

Calculate a [logarithm][] with base 10. If you need a logarithm of a value 'x'
to any other base, base 'b', you can use:

        logb(x) = log10(x) / log10(b)
        or
        logb(x) = log(x) / log(b)

        # (log10 ARITH)
        > (log10 e)
        0.434294
        > (log10 1)
        0.0
        > (log10 -1)
        nan
        > (log10 10)
        1.0
        > (log10 3.3)
        0.518514

* fabs

Return the [absolute value][] of a floating point number.

        # (fabs ARITH)
        > (fabs 1)
        1
        > (fabs -2)
        2
        > (fabs 5.2)
        5.2
        > (fabs -4.0)
        4.0

* sin

Calculate the [sine][] of an angle, in radians.

        # (sin ARITH)
        > (sin pi)
        0.0
        > (sin (/ pi 2))
        1.0

* cos

Calculate the [cosine][] of an angle, in radians.

        # (cos ARITH)
        > (cos pi)
        -1.0
        > (cos (/ pi 2))
        0.0
        > (cos (/ pi 3))
        0.5

* tan

Calculate the [tangent][] of an angle, in radians.

        # (tan ARITH)
        > (tan 0)
        0
        > (tan (/ pi 3))
        1.732051

* asin

Calculate the [reciprocal of sine][], or the "cosecant".

        # (asin ARITH)
        > (asin 0)
        0.0
        > (asin 1.0)
        1.570796

* acos

Calculate the [reciprocal of cosine][], or the "secant".

        # (acos ARITH)
        > (acos -1)
        3.141593
        > (acos 0.0)
        1.570796

* atan

Calculate the [reciprocal of tangent][], or the "cotangent".

        (atan ARITH)

* sinh

Calculate the [hyperbolic sine][].

        (sinh ARITH)

* cosh

Calculate the [hyperbolic cosine][].

        (cosh ARITH)

* tanh

Calculate the [hyperbolic tangent][].

        (tanh ARITH)

* exp

Calculate the [exponential function][]. Or Euler's number raised to the
floating point number provided.

        # (exp ARITH)
        > (exp 0)
        1.0
        > (exp 1)
        2.718282
        > (exp -1)
        0.367879
        > (exp 2)
        7.389056
        > (exp 0.5)
        1.648721

* sqrt

Calculate the [square root][] of a number.

        # (sqrt ARITH)
        > (sqrt 100)
        10.0
        > (sqrt 69.0)
        8.306624
        > (sqrt -4)
        -nan
        > (sqrt 4)
        2.0

* ceil

Calculate the [ceil][] of a float, or round up to the nearest integer, from
"ceiling".

        # (ceil ARITH)
        > (ceil 4)
        4
        > (ceil 4.1)
        5.0
        > (ceil -3)
        -3.0
        > (ceil -3.1)
        -3.0

* floor

Round down to the "[floor][]", or down to the nearest integer.

        # (floor ARITH)
        > (floor 4)
        4.0
        > (floor 4.9)
        4.0
        > (floor -3)
        -3.0
        > (floor -4.9)
        -5.0

* pow

Raise the first value to the power of the second value.

        # (pow ARITH ARITH)
        > (pow 2 4)
        16.0
        > (pow -2  0.5)
        1.414214
        > (pow -2 -0.5)
        0.707107
        > (pow -2 -0.5)
        nan

* [modf][]

Split a floating point value (integers are converted to floats first) into
integer and fractional parts, returning a cons of the two values.

        # (modf ARITH)
        > (modf 2)
        (2.000000 . 0.000000)
        > (modf 2.1)
        (2.000000 . 0.100000)
        > (modf -3.5)
        (-3.000000 . -0.500000)
        > (modf -0.4)
        (-0.000000 . -0.400000)

* line-editor-mode

If the line editing library is used then this function can be used to query the
state of line editor mode, 't' representing ["Vi"][] like editing mode, 'nil'
["Emacs"][] mode. The mode can be changed by passing in 't' to set it to 'Vi'
mode and 'nil' to Emacs mode.

        (line-editor-mode)
        (line-editor-mode T-or-Nil)

* history-length

This variable controls the length of the history that the line-editing library
saves.

        # (history-length INT)
        > (history-length 1) # disable history
        > (history-length 2) # remember one entry
        > (history-length 200) # remember 199 enteries.

* clear-screen

Clear the terminal screen, in interactive mode the return value (always 't') is
printed and then the prompt if that option is set.

        # (clear-screen)
        > (clear-screen)
        t

#### Predefined variables

These are predefined variables used throughout the system, they often directly
relate to the internal C enumerations that are used, when this is the case they
are not guaranteed to be constant between different implementations or
versions, only self consistent with a single specific implementation.


* \*seek-cur\*

Option for *seek*. File seeks are performed from the current file position.

* \*seek-set\*

Option for *seek*. File seeks are performed relative to the beginning of the file.

* \*seek-end\*

Option for *seek*. File seeks are performed relative to the end of the file.

* \*random-max\*

Maximum number a random number generated by "(random)" can take.

* \*integer-max\*

Maximum number an integer can hold.

* \*integer-min\*

Minimum number an integer can hold.

The following 11 system variable represent the internal type that an object can
take. They are returned by functions like *type-of*;

For example:

        > (eq *integer* (type-of 1))
        t
        > (eq *integer* (type-of 1.0))
        nil

1.  \*integer\*, an integer, such as '1'
2.  \*symbol\*, a symbol such as '+'
3.  \*cons\*, a list such as '(1 2 3)'
4.  \*string\*, a string such as '"hello"'
5.  \*hash\*, a hash, made by *hash-create*
6.  \*io\*, an IO port, made by *open*
7.  \*float\*, a floating point number such as '2.0'.
8.  \*procedure\*, a procedure, such as "(lambda (x) (\* x x))"
9.  \*primitive\*, a built in subroutine written in C
10. \*f-procedure\*, a F-Expression, such as "(flambda (x) x)"
11. \*user-defined\*, an arbitrary user defined type

* \*file-in\*

An option for *open*. The string passed to open will be treated as a file name
to open up for reading.

        > (open *file-in* "file.txt")

* \*file-out\*

An option for *open*. The string passed to open will be treated as a file name
to open up for writing.

        > (open *file-out* "file.txt")

* \*string-in\*

An option for *open*. The string passed ot open will be treated as a string to
read from.

        > (define in (open *string-in* "(+ 2 2) \"Hello, World\""))
        <IO:IN:32145072>
        > (read in)
        (+ 2 2)
        > (read in)
        "Hello, World"
        > (read in)
        error

* \*string-out\*

An option for *open*. Not used for the moment.

* \*lc-all\*

An option for *locale*. It is used to set the entire locale.

* \*lc-collate\*

An option for *locale*. This is used to set the LC\_COLLATE option, which
affects *strcoll* and *strxfrm*, although they are not used within the
interpreter.

* \*lc-ctype\*

An option for *locale*. Affects character functions in "ctype.h", allowing the
locale to be changed for them.

* \*lc-monetary\*

An option for *locale*. Affects the money format.

* \*lc-numeric\*

An option for *locale*. Affects non monetary numbers in I/O functions like
scanf and printf, although this should not affect the interpreter.

* \*lc-time\*

An option for *locale*. Affects strftime, which is not used in the interpreter.

* \*trace-off\*

An option for *trace-level!*. Tracing is turned off for all objects.

        > (trace-level! *trace-off*)

* \*trace-marked\*

An option for *trace-level!*. Only marked objects will be traced.

        > (trace-level! *trace-marked*)

* \*trace-all\*

An option for *trace-level!*. All objects will be traced.

        > (trace-level! *trace-all*)

* \*gc-on\*

An option for *gc*. Turn the garbage collection back on if it was postponed.

        > (gc *gc-on*)

* \*gc-postpone\*

An option for *gc*. Postpone garbage collection.

        > (gc *gc-postpone*)

* \*gc-off\*

An option for *gc*. Turn off the garbage collector and **leak memory**.

        > (gc *gc-off*)


* \*eof\*

Represents the End-Of-File marker, can be passed into "get-delim" as a
delimiter.

* \*args\*

This is a list containing all of the arguments passed into the interpreter, it
is a list of strings, each string being a single argument.

If the interpreter was invoked like this

        ./lisp -Epc init.lsp

Then \*args\* will be:

        ("./lisp" "-Epc" "init.lsp")

\*args\* is only set at startup.

* \*history-file\*

The name of the history file in use by "libline", which is the line editor, if
that functionality has been added to the interpreter.

* \*version\*

This contains the version, as determined by the [VCS][], if the [VCS][] is not
available it will be set to "unknown".

* \*commit\*

This contains the commit, as determined by the [VCS][], if the [VCS][] is not
available it will be set to "unknown".

* \*repository-origin\*

This contains a string describing the origin of the lisp interpreters
repository, if this is not available it will be set to "unknown".

        # floats
        pi                 The mathematical constant pi
        e                  Euler's number

* \*have-compile\*

This is 't if the run time C compiler is available (backed by the Tiny C
Compiler), this is '() if it is not available. This feature currently is not
stable.

* \*have-line\*

This is 't if the line editor functions and variables are available,
it is '() otherwise.

* \*have-dynamic-loader\*

This is 't if the dynamic loader is available, it is '() otherwise.

* pi

Roughly "3.14159...".

The numerical constant ["pi"][], this is a floating point value that does not
follow the normal naming convention for the predefined variables of enclosing
the name within asterisks.

* e

Roughly "2.71828...".

The numerical constant ["e"][]. It also breaks with the usual variable naming
convention like ["pi"][] does.

#### Glossary

Glossary of all of defined subroutine primitives and variables.

##### Built in subroutines

        &                  Perform a bitwise AND on two integers
        |                  Perform a bitwise OR on two integers
        ^                  Perform a bitwise XOR on two integers
        ~                  Perform a bitwise inversion on an integer
        +                  Add two numbers
        -                  Subtract a number from another number
        *                  Multiply two numbers together
        %                  Calculate the remainder of a division
        /                  Divide two numbers
        =                  Test for equality
        eq                 Test for equality
        >                  Greater than test
        <                  Less than test
        cons               Create a new cons cell
        car                Get the first value of a cons cell
        cdr                Get the second value of a cons cell
        list               Create a list from multiple expressions
        match              Match a string based on a patten
        scons              Concatenate two strings
        scar               Get the first character in a string
        scdr               Get a string excluding the first character
        eval               Evaluate an expression
        trace-level!       Set the trace level
        gc                 Control the garbage collector
        length             Get the length of an expression or string
        input?             Check if an IO object is set up for reading
        output?            Check if an IO object is set up for writing
        eof?               Check if an IO object has its End Of File marker set
        flush              Flush an output stream
        tell               Get the current file position indicator
        seek               Perform a seek in a file
        open               Open an IO object for reading or writing (not both)
        get-char           Get a character
        get-delim          Read in a line delimited by a character
        read               Read in a S-Expression
        put                Print a string, unescaping it
        put-char           Print a character
        print              Print out an S-Expression
        ferror             Check if an IO object has encountered an error
        system             Run a command in the systems command processor
        remove             Remove files
        rename             Rename files
        all-symbols        Return all the symbols encountered (bound or not)
        hash-create        Create a hash table
        hash-lookup        Lookup a value in a hash by string
        hash-insert        Insert a value into a hash
        coerce             Convert types
        time               Return the time
        getenv             Get an environment variable
        random             The Pseudo Random Number Generator
        seed               Seed the Pseudo Random Number Generator
        date               Return a list of integers representing the date
        assoc              Find a value in an association list
        locale!            Set the locale
        trace              Control the tracing of an object
        binary-logarithm   Calculate the binary logarithm of an integer
        close              Close an IO object
        type-of            Return an integer-enum for the type of an object
        timed-eval         Eval with execution time
        reverse            Reverse a string, a list or a hash
        split              Split a string based
        join               Join a list of strings together
        substring          Make a substring from a string
        format             Print out a list of objects based on a format string
        regex              Match a regular expression on a string
        raise              Raise a signal
        isalnum?           is string or integer alphanumeric only?
        isalpha?           is string or integer alphabetic only?
        iscntrl?           is string or integer control character?
        isdigit?           is string or integer digits?
        isgraph?           is string or integer printable (except space)?
        islower?           is string or integer is lower case?
        isprint?           is string or integer is printable?
        ispunct?           is string or integer is punctuation?
        isspace?           is string or integer is white space?
        isupper?           is string or integer is upper case?
        isxdigit?          is string or integer is a hex digit?
        tr                 translate a string

##### math.h

        log                Compute the natural logarithm of a float
        log10              Compute the logarith of a float in base 10
        fabs               Return the absolute
        sin                Sine of an angle (in radians)
        cos                Cosine of an angle (in radians)
        tan                Tangent of an angle (in radians)
        asin               Arc Sine
        acos               Arc Cosine
        atan               Arc Tangent
        sinh               Hyperbolic Sine
        cosh               Hyperbolic Cosine
        tanh               Hyperbolic Tangent
        exp                Eulers constant raised to the power of X
        sqrt               Squareroot of a float
        ceil               Round upwards
        floor              Round downwards
        pow                Computer X raised to the Y
        modf               Split a value into integer and fractional parts

##### compiler 

* compile

* link

* compile-file

* get-subroutine

* add-include-path

* add-system-include-path

* set-library-path

##### dynamic loader

This is the interface to the dynamic module loader. This can be used to load
compiled modules at run time so the interpreter can access compiled subroutines
after the interpreter itself has been compiled. This interface does not
implement a way to close loaded modules as there is no way to invalidate
previously acquired subroutines.

* dynamic-open

Open up a module for use, it returns a user defined type representing the
shared library handle that can be used with *dynamic-symbol*.

* dynamic-symbol

Get a symbol from a compiled module, the symbol should be a function abiding by
the function type of all the built in subroutines. The following C typedef is
how the returned subroutine should look like:

        typedef cell *(*subr)(lisp *, cell*);

This function will return 'error if the subroutine could not be found (or there
was another error, which one can be found with dynamic-error).

Below is an example, given a shared module "libexample.so", with the example
code:

        #include <liblisp.h>
        cell* subr_example(lisp *l, cell *args) {
                if(!cklen(args, 1))
                        RECOVER(l, "\"expected (int)\" '%S", args);
                return mkint(l, intval(car(args)) * 2);
        }

Make sure ldconfig has been run after installing libexample.so into a directory
that the loader can find it, the following code in the interpreter will load
that shared library and allow the function to be run.

        # (dynamic-symbol DYNAMIC-MODULE STRING)
        > (define a-module (dynamic-open "libexample.so"))
        <DYNAMIC-MODULE:34437120>
        > (define example-subr (dynamic-symbol a-module "subr_example"))
        <SUBR:47982636828560>
        > (example-subr 4)
        8
        > (example-subr -4)
        -8
        
* dynamic-error

This function returns an error string representing the latest error to occur
(if any) resulting from an error with the dynamic loader. 

        > (define a-module (dynamic-open "libexample.so"))
        <DYNAMIC-MODULE:34437120>
        > (dynamic-symbol a-module "subr_does_not_exist")
        error
        > (dynamic-error)
        "libdoesnotexist.so: cannot open shared object file: No such file or directory"


##### line-editor (optional)

        line-editor-mode   Change the line editing mode
        history-length     Change the number of records stored in the history file
        clear-screen       Clear the screen

##### integers

        *seek-cur*         Seek from current file marker with seek
        *seek-set*         Seek from the beginning of a file with seek
        *seek-end*         Seek from the end of a file with seek
        *random-max*       Maximum number a random number can be
        *integer-max*      Maximum number an integer can be
        *integer-min*      Minimum number an integer can be
        *integer*          Integer   type option for coerce or type-of
        *symbol*           Symbol    type option for coerce or type-of
        *cons*             Cons      type option for coerce or type-of
        *string*           String    type option for coerce or type-of
        *hash*             Hash      type option for coerce or type-of
        *io*               IO        type option for type-of
        *float*            Float     type option for coerce or type-of
        *procedure*        Procedure type option type-of
        *primitive*        Primitive type option for type-of
        *f-procedure*      Fexpr     type option for type-of
        *user-defined*     User-define type option for type-of
        *file-in*          File   input  option for open
        *file-out*         File   output option for open
        *string-in*        String input  option for open
        *string-out*       String output option for open
        *lc-all*           Locale all option for locale!
        *lc-collate*       Locale collate option for locale!
        *lc-ctype*         Locale C-type option for locale!
        *lc-monetary*      Locale money formatting option for locale!
        *lc-numeric*       Locale numeric option for locale!
        *lc-time*          Locale time option for locale!
        *trace-off*        Option for trace, turn it off
        *trace-marked*     Option for trace, trace only marked objects
        *trace-all*        Option for trace, trace all expression evaluations
        *gc-on*            Option for gc, Turn Garbage Collection on
        *gc-postpone*      Option for gc, Postpone Garbage Collection, temporarily
        *gc-off*           Option for gc, Turn off Garbage Collection permanently
        *eof*              End-Of-File marker
        *args*             Command line options passed into the interpreter
        *history-file*     If using libline, it contains the history file name
        *have-compile*     't if the C compiler is available, '() otherwise
        *have-line*        't if the line editor is available, '() otherwise
        *have-dynamic-loader* 't if the dynamic loader is available, '() otherwise
        *version*          The version of this interpreter if known
        *commit*           Commit version
        *repository-origin* Origin of the lisp interpreters repository

##### floats
        pi                 The mathematical constant pi
        e                  Euler's number

<div id='References'/>
## References

A list of references used for this lisp interpreter and this documentation.

### X-Macros

X-Macros are heavily used within the interpreter for initialization of the data
structures relating to built in subroutines, special symbols (such as *nil*,
*t*, *cond*, *if*, and others) and normal symbols (such as *pi*, *e* and
*integer-max*) that are useful.

1. <https://en.wikipedia.org/wiki/X_Macro>
2. <http://www.drdobbs.com/the-new-c-x-macros/184401387>
3. <http://www.drdobbs.com/cpp/the-x-macro/228700289>

### "Struct hack" and Flexible Array Members

The "Struct hack" in C is a common pre C99 technique that has been replaced by
Flexible Array Members. It does not quite work in this project however, due to
the fact that it needs to initialize static structures as well.

1. <http://c-faq.com/struct/structhack.html>
2. <https://stackoverflow.com/questions/16553542/c-struct-hack-at-work>
3. <https://stackoverflow.com/questions/3711233/is-the-struct-hack-technically-undefined-behavior>
4. <https://en.wikipedia.org/wiki/Flexible_array_member>
5. <https://stackoverflow.com/questions/246977/flexible-array-members-in-c-bad>

### [Lisp][]

Here are a list of references on how to implement lisp interpreters and to
other lisp interpreters people have made.

1.  <http://ldc.upenn.edu/myl/llog/jmc.pdf>
2.  <http://c2.com/cgi/wiki?ImplementingLisp>
3.  <http://www.sonoma.edu/users/l/luvisi/sl5.c>
4.  <https://news.ycombinator.com/item?id=8444930>
5.  <https://github.com/kimtg/Arcadia>
6.  <https://www.gnu.org/software/guile/>
7.  <https://github.com/kanaka/mal>
8.  <https://news.ycombinator.com/item?id=9121448>
9.  <http://www.schemers.org/Documents/Standards/R5RS/>
10. <https://news.ycombinator.com/item?id=869141>
11. <http://shrager.org/llisp/>
12. <https://github.com/darius/ichbins>

### Garbage Collection

Links on garbage collection in general:

1. <http://www.brpreiss.com/books/opus5/html/page424.html>
2. <http://c2.com/cgi/wiki?MarkAndSweep>

### Regex

The matching function provided by the base library "match" is very simple, but
can be improved upon by several regex implementations shown here.

1. <https://swtch.com/~rsc/regexp/regexp1.html>
2. <http://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html>
3. <http://c-faq.com/lib/regex.html>

### Bignum

Arbitrary precision arithmetic was investigated but it was decided instead to
use floating point numbers and integers instead.

1. <https://github.com/antirez/sbignum>
2. <http://www.mit.edu/afs.new/athena/contrib/crypto/src/SSLeay-0.6.4/crypto/bn/bn_knuth.c>
3. <https://www3.cs.stonybrook.edu/~skiena/392/programs/bignum.c>
4. <https://en.wikipedia.org/wiki/The_Art_of_Computer_Programming>

### Hashes

A hashing library is provided along with the interpreter, the interpreter makes
use of it.

1. <http://www.cse.yorku.ca/~oz/hash.html>
2. <https://en.wikipedia.org/wiki/Hash_table#Separate_chaining>

### String handling

Various string handling routines were adapted from the C-FAQ.

1. <http://c-faq.com/varargs/varargs1.html>

### Bit twiddling hacks

One routine from the "bignum" library that was left in that related to binary
logarithms was derived from here:

1. <http://graphics.stanford.edu/~seander/bithacks.html>

### Line editing library

The line editing library used in this document was called "linenoise", which is
a popular and minimal library for line editing released under a BSD license. I
have since made (fairly minor) modifications to the library and added support
for ["Vi"][] like editing and changed the API naming somewhat, the modified
library has been used in this project.

1. <https://github.com/antirez/linenoise>
2. <https://github.com/howerj/libline>

### Documentation

The CSS style sheet used in this document and the markdown to HTML converter
used:

1. <http://bettermotherfuckingwebsite.com/>
2. <https://daringfireball.net/projects/markdown/>

<!-- Links used throughout the document. These will not be present in the HTML
     but only in the markdown document used to generate the HTML -->
[side effects]: https://en.wikipedia.org/wiki/Side_effect_%28computer_science%29
[recursion]: https://en.wikipedia.org/wiki/Recursion_%28computer_science%29
[tail call]: https://en.wikipedia.org/wiki/Tail_call
[mark and sweep]: http://c2.com/cgi/wiki?MarkAndSweep
[MATLAB]: https://en.wikipedia.org/wiki/MATLAB
[AWK]: https://en.wikipedia.org/wiki/AWK
[S-Expressions]: https://en.wikipedia.org/wiki/S-expression
[S-Expression]: https://en.wikipedia.org/wiki/S-expression
[Garbage Collection]: https://en.wikipedia.org/wiki/Garbage_collection_%28computer_science%29
[Lisp]: https://en.wikipedia.org/wiki/Lisp_%28programming_language%29
[FORTRAN]: https://en.wikipedia.org/wiki/Fortran
[ANSI C]: https://en.wikipedia.org/wiki/ANSI_C
[c99]: https://en.wikipedia.org/wiki/C99
[Cygwin]: https://www.cygwin.com/
[VT220]: https://en.wikipedia.org/wiki/VT220
[ANSI escape codes]: https://en.wikipedia.org/wiki/ANSI_escape_code#Colors
[libline]: https://github.com/howerj/libline
[linenoise]: https://github.com/antirez/linenoise
["Vi"]: https://en.wikipedia.org/wiki/Vi
[liblisp.c]: liblisp.c
[liblisp.h]: liblisp.h
[liblisp.1]: liblisp.1
[liblisp.3]: liblisp.3
[init.lsp]:  init.lsp
[meta.lsp]:  meta.lsp
[test.lsp]:  test.lsp
[make.bat]:  make.bat
[main.c]: main.c
[metasyntactic variables]: http://www.catb.org/jargon/html/M/metasyntactic-variable.html
[fexpr]: https://en.wikipedia.org/wiki/Fexpr
[tail calls]: https://en.wikipedia.org/wiki/Tail_call
[lambda procedure]: https://en.wikipedia.org/wiki/Anonymous_function
["pi"]: https://en.wikipedia.org/wiki/Pi
["e"]: https://en.wikipedia.org/wiki/E_%28mathematical_constant%29
["Emacs"]: https://en.wikipedia.org/wiki/Emacs
[natural logarithm]: https://en.wikipedia.org/wiki/Natural_logarithm
[logarithm]: https://en.wikipedia.org/wiki/Logarithm
[absolute value]: https://en.wikipedia.org/wiki/Absolute_value
[sine]: https://en.wikipedia.org/wiki/Sine
[cosine]: https://en.wikipedia.org/wiki/Trigonometric_functions#Sine.2C_cosine_and_tangent
[tangent]:https://en.wikipedia.org/wiki/Trigonometric_functions#Sine.2C_cosine_and_tangent
[reciprocal of sine]: https://en.wikipedia.org/wiki/Trigonometric_functions#Reciprocal_functions
[reciprocal of cosine]: https://en.wikipedia.org/wiki/Trigonometric_functions#Reciprocal_functions
[reciprocal of tangent]: https://en.wikipedia.org/wiki/Trigonometric_functions#Reciprocal_functions
[hyperbolic sine]: https://en.wikipedia.org/wiki/Hyperbolic_function
[hyperbolic cosine]: https://en.wikipedia.org/wiki/Hyperbolic_function
[hyperbolic tangent]: https://en.wikipedia.org/wiki/Hyperbolic_function
[exponential function]: https://en.wikipedia.org/wiki/Exponential_function
[square root]: https://en.wikipedia.org/wiki/Square_root
[ceil]: http://www.cplusplus.com/reference/cmath/ceil/
[floor]: http://www.cplusplus.com/reference/cmath/floor/
[modf]: http://www.cplusplus.com/reference/cmath/modf/
[sets]: https://en.wikipedia.org/wiki/Set_theory
[factorial]: https://en.wikipedia.org/wiki/Factorial
[gcd]: https://en.wikipedia.org/wiki/Greatest_common_divisor
[Meta-circular Evaluator]: https://en.wikipedia.org/wiki/Meta-circular_evaluator
[The Roots of Lisp]: ldc.upenn.edu/myl/llog/jmc.pdf
[cons cell]: https://en.wikipedia.org/wiki/Cons
[associative arrays]: https://en.wikipedia.org/wiki/Associative_array
[dotted pairs]: http://c2.com/cgi/wiki?DottedPairNotation
[dotted pair]: http://c2.com/cgi/wiki?DottedPairNotation
[Trees]: https://en.wikipedia.org/wiki/Tree_%28data_structure%29
[Scheme]: https://en.wikipedia.org/wiki/Scheme_%28programming_language%29
[Common Lisp]: https://en.wikipedia.org/wiki/Common_Lisp
[REPL]: https://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop
[git]: https://git-scm.com/
[VCS]: https://en.wikipedia.org/wiki/Revision_control
<!-- This isn't meant to go here but it is out of the way -->
<style type="text/css">body{margin:40px auto;max-width:650px;line-height:1.6;font-size:18px;color:#444;padding:0 10px}h1,h2,h3,h4,h5,h6{line-height:1.2}</style>
