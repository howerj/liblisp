;;; c.lsp ;;;
;; This module gives example of a crude method of compiling
;; strings into functional "built-in" subroutines using the
;; Tiny C Compiler library.
;;
;; It would be better to pass in an S-Expression representing
;; the program to be compiled instead of strings, which are
;; clunky.

; Begin C functions definitions
(define square
  (cc *compile-state* 
    "square" ; name of the C function we want to extract 
    ; This is a simple C test function, it will square a number
    "#include <liblisp.h> /*liblisp needs to be installed and on include path*/
    cell *square(lisp *l, cell *args) { /*simple test function*/
          if(!cklen(args, 1) || !is_arith(car(args)))
                  RECOVER(l, \"\\\"expected (number)\\\" '%S \", args);
          if(is_float(car(args)))
              return mk_float(l, val_float(car(args)) * val_float(car(args)));
          else
              return mk_int(l, val_int(car(args)) * val_int(car(args)));
    }"))

