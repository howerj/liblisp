liblisp.md 
==========

A small and extensible lisp interpreter and library.

  
            _ _ _     _ _           
           | (_) |   | (_)          
           | |_| |__ | |_ ___ _ __  
           | | | '_ \| | / __| '_ \ 
           | | | |_) | | \__ \ |_) |
           |_|_|_.__/|_|_|___/ .__/ 
                             | |    
                             |_|    
 

@todo Write this manual, of course! This will require some stability in the
interpreter APIs and functionality first.
  
# The liblisp manual

This is the manual for the liblisp library and the small lisp interpreter it
provides. The provided lisp interpreter is a wrapper around the library with
some additional functionality for loading compiled modules in at run time so
the interpreter can be extended with C code.

### Design goals

This library has the following goals:

* Small
* Extensible
* Encapsulated in a library
* Written in pure, ISO C99 (originally ANSI C89).
* Provide general utilities useful for other, non-lisp related C programs.

### Known bugs

Any operation that causes a segmentation fault or corruption of data from
within the interpreter is definitely a bug 

* Recursive data structures cause problems when printed.
  - Until cycle detection is added, the only way to mitigate this is
  to not do it!
* Known bugs in the code are marked with "@bug" or "@todo".

#### Arbitrary limits

Arbitrary limits are to be considered bugs, and here is a list of known
limits within the interpreter:

* Limits on String and Symbol size are whatever can be index by intprt\_t on
your machine.
* Limits on argument length for functions is the same.

## Introduction
### LISP
### Interacting with the interpreter
#### Command Line Options

For the most up to date options, consult the manual pages for this program
instead of this manual, they are much more likely to be kept up to date.

 * -h

Print out the usage string and exit successfully.

 * -c

Turn on colorization of printed out S-Expressions, it assumes your terminal can
handle ANSI color codes. If it cannot do not select this option. It does no
checking for whether it is writing to terminal or not with isatty(3).

 * -p

This will turn the prompt on, by default no prompt is given when typing so as
to allow output to be piped to other programs following the "No news is good
news" Unix maxim.

 * -v

Increase the verbosity level of the lisp interpreter, by default the
interpreter tries to be as silent as possible. These messages will be
logged to stderr.

 * -V

Print out the version number and quit

 * -E

This will turn on the line editor when reading from stdin.

 * -H

If an error is encountered the interpreter will be halted rather than trying to
recover.

 * -i file

This option allows the user to supply a file name that would otherwise look
like an argument to the lisp interpreter, for example if the file name began
with a "-" character.

 * -- file
Same as "-i".

 * -e string
Evaluate a string as input, this disables reading from stdin(3).

 * -o file
Open "file" and redirect output to it.

 * -L
Use the default locale instead of the "C" locale, using setlocal(3).

 * file

An argument that is not preceded by a valid option that takes an argument is 
treated as a file to be read from. Reading from a single file disables reading
from stdin(3).

 * -

This forces reading from stdin(3). By default the interpreter will read from
stdin(3) but this behavior is disabled if an argument has been evaluated with
"-e" or a file has been read in.

If no file is given the interpreter reads input from stdin(3), if one is given
then the interpreter will exit after having read all input files or evaluated
all strings.

Arguments are read in order and executed as soon as they are encountered.

### Line editor
## Glossary of built in functions

## Build instructions and process

The build is primarily tested on Linux, and only infrequently tested on
Windows and so it much less likely to work on that platform. To build the
library a C compiler capable of compiling C99 is required, and GNU make is
required to use the provided makefile.

On Windows extra commands are required when compiling a DLL so the functions
get exported correctly.

All of the modules compile correctly on Linux when the right libraries and
headers are installed, only a subset of these modules have been tested and
built on Windows, this is reflected in the build system.

The core library is written purely in ISO C99, any deviation from the standard
is a unintentional bug. The operating system dependent code is captures in the
library wrapper and any compiled modules.

C99 is only use infrequently, the main uses are the new types provided in
"stdint.h", occasionally other C99 constructions are used as well.

#### Build options

### Linux
### Windows
## Modules and C API

The library provides a C API and the interpreter wrapper provides extra 
functions, that are not part of standard C, for use within any compiled
modules.

### C API

The library provides an API which can be used to parse, evaluate and print out
lisp code, it can also be used to manipulate lisp data structures, and pass and
receive them from an interpreter. Also provided are generic utility functions
that were made for use in the lisp interpreter, such as a hash implementation.

#### Example programs
### Making a C module

<!-- This isn't meant to go here but it is out of the way -->
<style type="text/css">body{margin:40px auto;max-width:650px;line-height:1.6;font-size:18px;color:#444;padding:0 10px}h1,h2,h3,h4,h5,h6{line-height:1.2}</style>
