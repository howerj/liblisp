# liblisp.md
# A small and extensible lisp interpreter and library
## Table of contents

1. [Introduction](#Introduction)
2. [Design Goals](#Design goals)
3. [Documentation](#Documentation)
4. [Building](#Building)
5. [Deviations from other lisps](#Deviations from other lisps)
6. [BUGS](#BUGS)
7. [Test programs](#Test programs)
8. [Manual](#Manual)
9. [References](#References)

<div id='Introduction'/>
## Introduction

This is a lisp interpreter written largely in [ANSI C][], with some [c99][], it is a 
non-standard variant of lisp. 

<div id='Design goals'/>
## Design goals

A summary of the design goals:

* Simple and Small (< 3000 lines of code)
* Extensible
* No implicit state
* Used as a generic utility library

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
design, which impedes multiple instances running at once.

* Used as a generic utility library

The library includes routines that might be useful in other programs that it
could have chosen not to expose, mostly relating to I/O and strings.

Most of these goals have been met, although there is some implicit state
which is regrettable. The implicit state comes from signal handler for SIGINT,
this signal handler is used in the REPL function that has been provided. 

It is unlikely that a user of this library would want to instantiate two 
instances of that REPL within the same program, it would far easier to simply
run two instances of the program.

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

        gcc   liblisp.c main.c -o lisp
        tcc   liblisp.c main.c -o lisp
        clang liblisp.c main.c -o lisp

or the following to include basic mathematical functions:

        gcc   liblisp.c -DUSE_MATH main.c -lm -o lisp
        tcc   liblisp.c -DUSE_MATH main.c -lm -o lisp
        clang liblisp.c -DUSE_MATH main.c -lm -o lisp

The preprocessor defines that add or change functionality are

        USE_MATH        Add mathematical functions from the C library
        USE_LINE        Add a line editor, must be linked with libline.a
        NDEBUG          The interpreter liberally uses assertions

Using the makefile instead (either "make" or "make all") will build the "lisp"
executable as well as the "liblisp.a" library. It will not build "liblisp.so"
by default, instead "make liblisp.so" will do that. The libraries will be built
with debugging symbols.

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

* '#' as comments

Another major deviation is that comments are represented by '#' instead of ';'
so that source code can easily be run as a Unix script with no special syntax
in the usual manner.

<div id='BUGS'/>
## BUGS

* If you manage to get a SEGFAULT then there is a bug in the code somewhere.
* See the "@todo" comments at the top of [liblisp.c][] to see a full list of
  bugs as well comments labeled with "XXX".

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
                cout << "(debug 'cpp-test)" << endl;
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

### Lisp

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

### An introduction to the interpreter

### An introduction to the language

### Fexprs

An [fexpr][] is a function, user defined or an internal, whose arguments are
passed into it unevaluated, allowing them to be evaluated or many times or not
at all within the [fexpr][]. They have fallen out of favor within lisps as they
defy code optimization techniques which is not a concern in this implementation
as none are performed (beyond optimizing [tail calls][]).

### User defined types

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

### Example code

The library has example code in the repository it is hosted in:

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
        error              Error, an object representing an error

##### Special variables

There are several special variables used within the interpreter.

* nil

Represents false and the end of a list. Can also be written as '()'.

* t

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
the symbols defined in the current and encapsulating scopes.

        > (environment)
        ((x . 4) 
         ...
         (*seek-cur* . 1) 
         (e . 2.718282) 
         (pi . 3.141593) 
         (t . t))

* let\*

Creates new temporary local bindings for variables and evaluates expressions
with them. The format is:

        (let (VARIABLE VALUE) (VARIABLE VALUE) ... EXPR)

        # Examples:
        > (let* (x 3) (y (+ x x)) y)
        6
        > (define baz (lambda (y) (let* (foo -2) (bar 2) (+ (* foo y) bar))))
        > (baz 5)
        -8

* error

The error special form can be used to throw errors, it is also used as a
representation of an error returned from primitive functions.

        (error)
        (error INTEGER)

#### Built in subroutines

The following is a list of all the built in subroutines that are always
available.

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
        type?              Check the type of an object
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

* &

Perform the bitwise and of two integers.

        (& INT INT)

* |

Perform the bitwise or of two integers.

        (| INT INT)

* ^

Perform the bitwise exclusive of two integers.

        (^ INT INT)

* ~

Perform the bitwise inversion of an integer.

        (~ INT)

* \+

Add two numbers together.

        (+ ARITH ARITH)

* \-

Subtract the second argument from the first.

        (- ARITH ARITH)

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

Test for equality between two expressions.

        (= EXPR EXPR)

* eq

Test for equality between two expressions.

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

        (match STRING STRING)

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

        (length LIST)
        (length STRING)

* type?

        (type? ENUM EXPR)

* input?

        (input? EXPR)

* output?

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
is used at the moment as a delimiter.

        (get-delim STRING)
        (get-delim IN STRING)

* read

Read in an S-Expression, if no input port is given it will use the standard IO
input port. This returns an S-Expression, not a string.

        (read)
        (read IN)

* put

Write a string to an IO output port, if no IO port is given it will use the
standard output IO port (which may be different from stdout).

        (put STRING)
        (put OUT STRING)

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

        (system)
        (system STRING)

* remove

        (remove STRING STRING)

* rename

        (rename STRING STRING)

* all-symbols

        (all-symbols)

* hash-create

Create a new list from a series of key-value pairs. The number of arguments to
the function must be even (which includes zero).

        (hash-create)
        (hash-create {STRING EXPR}... )

* hash-lookup

Lookup a string in a hash.

        (hash-lookup HASH STRING)

* hash-insert

Insert a key-value pair into a hash table.

        (hash-insert HASH SYMBOL EXPR)

* coerce

Coerce one type into another.

        (coerce ENUM EXPR)

* time

Return a number representing the time defined by the implementation of the C
library that this program was linked against.

        (time)

* getenv

Get an environment variable from the system based on a string.

        (getenv STRING)

* random

Return a number from the Pseudo Random Number Generator.

        (random)

* seed

Seed the random number generator.

        (seed INT INT)

* date

Return a list representing the date.

        (date)

* assoc

Find an atom within an association list.

        (assoc ATOM A-LIST)

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

        (binary-logarithm INT)

* close

Close an IO port.

        (close IO)

##### Additional functions

Functions added optionally in [main.c][]:

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
        line-editor-mode   Change the line editing mode

For all the mathematical functions imported from the C math library the
arguments are converted to floating point numbers before, the functions also
all return floating point values, apart from [modf][] which returns a cons of
two floating point values.

* log

Calculate the [natural logarithm][] of a floating point value. Integers are
converted to floating point values before hand.

        (log ARITH)

* log10

Calculate a [logarithm][] with base 10. If you need a logarithm of a value 'x' 
to any other base, base 'b', you can use:

        logb(x) = log10(x) / log10(b)
        or
        logb(x) = log(x) / log(b)

        (log10 ARITH)

* fabs

Return the [absolute value][] of a floating point number.

        (fabs ARITH)

* sin

Calculate the [sine][] of an angle, in radians.

        (sin ARITH)

* cos

Calculate the [cosine][] of an angle, in radians.

        (cos ARITH)

* tan

Calculate the [tangent][] of an angle, in radians.

        (tan ARITH)

* asin

Calculate the [reciprocal of sine][], or the "cosecant".

        (asin ARITH)

* acos

Calculate the [reciprocal of cosine][], or the "secant".

        (acos ARITH)

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

        (exp ARITH)

* sqrt

Calculate the [square root][] of a number.

        (sqrt ARITH)

* ceil

Calculate the [ceil][] of a float, or round up to the nearest integer, from
"ceiling".

        (ceil ARITH)

* floor

Round down to the "[floor][]", or down to the nearest integer.

        (floor ARITH)

* pow

Raise the first value to the power of the second value.

        (pow ARITH ARITH)

* [modf][]

Split a floating point value (integers are converted to floats first) into
integer and fractional parts, returning a cons of the two values.

        (modf ARITH)

* line-editor-mode

If the line editing library is used then this function can be used to query the
state of line editor mode, 't' representing ["Vi"][] like editing mode, 'nil'
["Emacs"][] mode. The mode can be changed by passing in 't' to set it to 'Vi' 
mode and 'nil' to Emacs mode.

        (line-editor-mode)
        (line-editor-mode T-or-Nil)

#### Predefined variables

        # integers
        *seek-cur*         Seek from current file marker with seek
        *seek-set*         Seek from the beginning of a file with seek
        *seek-end*         Seek from the end of a file with seek
        *random-max*       Maximum number a random number can be
        *integer-max*      Maximum number an integer can be
        *integer-min*      Minimum number an integer can be
        *integer*          Integer type option for coerce or type?
        *symbol*           Symbol  type option for coerce or type?
        *cons*             Cons    type option for coerce or type?
        *string*           String  type option for coerce or type?
        *hash*             Hash    type option for coerce or type?
        *io*               IO      type option for type?
        *float*            Float   type option for coerce or type?
        *procedure*        Procedure type option type?
        *primitive*        Primitive type option for type?
        *f-procedure*      Fexpr     type option for type?
        *user-defined*     User-define type option for type?
        *file-in*          File input option for open
        *file-out*         File output option for open
        *string-in*        String input option for open
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

* \*seek-cur\*
* \*seek-set\*
* \*seek-end\*
* \*random-max\*
* \*integer-max\*
* \*integer-min\*
* \*integer\*
* \*symbol\*
* \*cons\*
* \*string\*
* \*hash\*
* \*io\*
* \*float\*
* \*procedure\*
* \*primitive\*
* \*f-procedure\*
* \*user-defined\*
* \*file-in\*
* \*file-out\*
* \*string-in\*
* \*string-out\*
* \*lc-all\*
* \*lc-collate\*
* \*lc-ctype\*
* \*lc-monetary\*
* \*lc-numeric\*
* \*lc-time\*
* \*trace-off\*
* \*trace-marked\*
* \*trace-all\*
* \*gc-on\*
* \*gc-postpone\*
* \*gc-off\*

        # floats
        pi                 The mathematical constant pi
        e                  Euler's number

* pi

Roughly "3.14159...".

The numerical constant ["pi"][], this is a floating point value that does not
follow the normal naming convention for the predefined variables of enclosing
the name within asterisks. 

* e

Roughly "2.71828...".

The numerical constant ["e"][]. It also breaks with the usual variable naming
convention like ["pi"][] does.

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
 
### Lisp

Here are a list of references on how to implement lisp interpreters and to
other lisp interpreters people have made.

1. <http://ldc.upenn.edu/myl/llog/jmc.pdf>
2. <http://c2.com/cgi/wiki?ImplementingLisp>
3. <http://www.sonoma.edu/users/l/luvisi/sl5.c>
4. <https://news.ycombinator.com/item?id=8444930>
5. <https://github.com/kimtg/Arcadia>
6. <https://www.gnu.org/software/guile/>
7. <https://github.com/kanaka/mal>
8. <https://news.ycombinator.com/item?id=9121448>
9. <http://www.schemers.org/Documents/Standards/R5RS/>
10. <https://news.ycombinator.com/item?id=869141>

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
[ANSI C]: <https://en.wikipedia.org/wiki/ANSI_C>
[c99]: <https://en.wikipedia.org/wiki/C99>
[Cygwin]: <https://www.cygwin.com/>
[VT220]: <https://en.wikipedia.org/wiki/VT220>
[ANSI escape codes]: <https://en.wikipedia.org/wiki/ANSI_escape_code#Colors>
[libline]: <https://github.com/howerj/libline>
[linenoise]: <https://github.com/antirez/linenoise>
["Vi"]: <https://en.wikipedia.org/wiki/Vi>
[liblisp.c]: liblisp.c
[liblisp.h]: liblisp.h
[liblisp.1]: liblisp.1
[liblisp.3]: liblisp.3
[main.c]: main.c
[metasyntactic variables]: <http://www.catb.org/jargon/html/M/metasyntactic-variable.html>
[fexpr]: <https://en.wikipedia.org/wiki/Fexpr>
[tail calls]: <https://en.wikipedia.org/wiki/Tail_call>
[lambda procedure]: <https://en.wikipedia.org/wiki/Anonymous_function>
["pi"]: <https://en.wikipedia.org/wiki/Pi>
["e"]: <https://en.wikipedia.org/wiki/E_%28mathematical_constant%29>
["Emacs"]: <https://en.wikipedia.org/wiki/Emacs>
[natural logarithm]: <https://en.wikipedia.org/wiki/Natural_logarithm>
[logarithm]: <https://en.wikipedia.org/wiki/Logarithm>
[absolute value]: <https://en.wikipedia.org/wiki/Absolute_value>
[sine]: <https://en.wikipedia.org/wiki/Sine>
[cosine]: <https://en.wikipedia.org/wiki/Trigonometric_functions#Sine.2C_cosine_and_tangent>
[tangent]:<https://en.wikipedia.org/wiki/Trigonometric_functions#Sine.2C_cosine_and_tangent>
[reciprocal of sine]: <https://en.wikipedia.org/wiki/Trigonometric_functions#Reciprocal_functions>
[reciprocal of cosine]: <https://en.wikipedia.org/wiki/Trigonometric_functions#Reciprocal_functions>
[reciprocal of tangent]: <https://en.wikipedia.org/wiki/Trigonometric_functions#Reciprocal_functions>
[hyperbolic sine]: <https://en.wikipedia.org/wiki/Hyperbolic_function>
[hyperbolic cosine]: <https://en.wikipedia.org/wiki/Hyperbolic_function>
[hyperbolic tangent]: <https://en.wikipedia.org/wiki/Hyperbolic_function>
[exponential function]: <https://en.wikipedia.org/wiki/Exponential_function>
[square root]: <https://en.wikipedia.org/wiki/Square_root>
[ceil]: <http://www.cplusplus.com/reference/cmath/ceil/>
[floor]: <http://www.cplusplus.com/reference/cmath/floor/>
[modf]: <http://www.cplusplus.com/reference/cmath/modf/>
<!-- This isn't meant to go here but it is out of the way -->
<style type="text/css">body{margin:40px auto;max-width:650px;line-height:1.6;font-size:18px;color:#444;padding:0 10px}h1,h2,h3{line-height:1.2}</style>
