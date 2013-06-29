Implementation of the interpreter:
  S-expression parser.
    This really needs stress testing and reorganizing.
  Data structures.
  Replace fprintf with print_string which will call wrap_put.
  All functions check for null pointers.
  Print out all error messages as s-expressions.
  Check stack-depth, wimp out after limit reached.

Core functions to do:

7 functions:

    (quote x)
    (atom x)
    (eq x y)
    (cons x y)
    (cond (x y) (w z) (t q))
    (car x)
    (cdr x)
