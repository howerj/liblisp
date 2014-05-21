# plan.md

The first attempt at the lisp interpreter will be to just get one up and no
matter how inefficient. It will also help me to build up libraries that I can
reuse in a next attempt, explore ideas (proper tail recursion, garbage
collection, lisp, text processing) that will allow me to make better choices in
the next iteration.

## Possibly relevant links

Regex matching:

<http://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html>

Pretty Printing Trees:

<http://homepages.inf.ed.ac.uk/wadler/papers/prettier/prettier.pdf>

Bignums:

<http://www.cs.sunysb.edu/~skiena/392/programs/bignum.c> 

(from the book *Programming Challenges: The Programming Contest Training
 Manual*)


## Steps

The steps and sections I need to work on are:

### Garbage Collection

**DONE**

Add simple garbage collection so we do not end up taking up all the memory!


### Add config file

A simple configuration file would be nice, named *config.lsp* or something, it
would be easy enough to do given the already available parser/evaluator.

Configuration for:
* Default file(s) to read in before start up
* Colors for various symbols
* Set command line args automatically

### Text processing primitives and file I/O

I would like to make file I/O and text processing easy like they are in perl,
awk and sed.

### Colorize output and pretty print it

**DONE**

Pretty printing S-expressions should be relatively easy, colorizing the output
on demand as well could make any output easier to read. 

### Relatively Efficient

By better choice of data structures and algorithms, as well as architectural
decisions, I can implement a more efficient interpreter. I should move from an
array based approached to linked lists to help with insertion / deletion and
processing of environments.

### Bignum support

Support for bignums! The only type of number lisps need! Currently only signed
integers are supported and this is rather limiting.

### Easy system command execution, text parsing, text manipulation and file I/O

Much like Perl, ease of text manipulation with built-in "sed", "tr", etc.,
Simple file I/O and system command execution with things like back ticks make
the language instantly more usable.

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

### Get this working in it:

 (define (eval exp env)
   (cond ((self-evaluating? exp) exp)
         ((variable? exp) (lookup-variable-value exp env))
         ((quoted? exp) (text-of-quotation exp))
         ((assignment? exp) (eval-assignment exp env))
         ((definition? exp) (eval-definition exp env))
         ((if? exp) (eval-if exp env))
         ((lambda? exp)
          (make-procedure (lambda-parameters exp)
                          (lambda-body exp)
                          env))
         ((begin? exp)
          (eval-sequence (begin-actions exp) env))
         ((cond? exp) (eval (cond->if exp) env))
         ((application? exp)
          (apply (eval (operator exp) env)
                 (list-of-values (operands exp) env)))
         (else
          (error "Unknown expression type - EVAL" exp))))


 (define (apply procedure arguments)
   (cond ((primitive-procedure? procedure)
          (apply-primitive-procedure procedure arguments))
         ((compound-procedure? procedure)
          (eval-sequence
            (procedure-body procedure)
            (extend-environment
              (procedure-parameters procedure)
              arguments
              (procedure-environment procedure))))
         (else
          (error
           "Unknown procedure type - APPLY" procedure))))


