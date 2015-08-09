liblisp.md {#mainpage}
==========

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

The following functionality would be nice to put in to the core interpreter;
Support for matrices and complex numbers (c99 \_Complex), UTF-8 support, text
processing (diff, tr, grep, sed, à la perl), map and looping constructs
from scheme, doc-strings, optional parsing of strings and float, apply,
gensym, proper constant and a prolog engine.

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

        gcc   liblisp.c main.c -o lisp
        tcc   liblisp.c main.c -o lisp
        clang liblisp.c main.c -o lisp

The preprocessor defines that add or change functionality are:

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

* If you manage to get a SEGFAULT then there is a bug in the code somewhere. It
  should not be possible to make the interpreter seg-fault.
* See the "@todo" comments at the top of [liblisp.c][] to see a full list of
  bugs as well comments labeled with "@bug".

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
11. <http://shrager.org/llisp/>

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
[side effects]: <https://en.wikipedia.org/wiki/Side_effect_%28computer_science%29>
[recursion]: <https://en.wikipedia.org/wiki/Recursion_%28computer_science%29>
[tail call]: <https://en.wikipedia.org/wiki/Tail_call>
[mark and sweep]: <http://c2.com/cgi/wiki?MarkAndSweep>
[MATLAB]: <https://en.wikipedia.org/wiki/MATLAB>
[AWK]: <https://en.wikipedia.org/wiki/AWK>
[S-Expressions]: <https://en.wikipedia.org/wiki/S-expression>
[S-Expression]: <https://en.wikipedia.org/wiki/S-expression>
[Garbage Collection]: <https://en.wikipedia.org/wiki/Garbage_collection_%28computer_science%29>
[Lisp]: <https://en.wikipedia.org/wiki/Lisp_%28programming_language%29>
[FORTRAN]: <https://en.wikipedia.org/wiki/Fortran>
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
[init.lsp]:  init.lsp
[meta.lsp]:  meta.lsp
[test.lsp]:  test.lsp
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
[sets]: <https://en.wikipedia.org/wiki/Set_theory>
[factorial]: <https://en.wikipedia.org/wiki/Factorial>
[gcd]: <https://en.wikipedia.org/wiki/Greatest_common_divisor>
[Meta-circular Evaluator]: <https://en.wikipedia.org/wiki/Meta-circular_evaluator>
[The Roots of Lisp]: <ldc.upenn.edu/myl/llog/jmc.pdf>
[cons cell]: <https://en.wikipedia.org/wiki/Cons>
[associative arrays]: <https://en.wikipedia.org/wiki/Associative_array>
[dotted pairs]: <http://c2.com/cgi/wiki?DottedPairNotation>
[dotted pair]: <http://c2.com/cgi/wiki?DottedPairNotation>
[Trees]: <https://en.wikipedia.org/wiki/Tree_%28data_structure%29>
[Scheme]: <https://en.wikipedia.org/wiki/Scheme_%28programming_language%29>
[Common Lisp]: <https://en.wikipedia.org/wiki/Common_Lisp>
[REPL]: <https://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop>
[git]: <https://git-scm.com/>
[VCS]: <https://en.wikipedia.org/wiki/Revision_control>
<!-- This isn't meant to go here but it is out of the way -->
<style type="text/css">body{margin:40px auto;max-width:650px;line-height:1.6;font-size:18px;color:#444;padding:0 10px}h1,h2,h3,h4,h5,h6{line-height:1.2}</style>
