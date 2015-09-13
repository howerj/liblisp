;;; c.lsp ;;;
;; This module gives example of a crude method of compiling
;; strings into functional "built-in" subroutines using the
;; Tiny C Compiler
;;
;; It would be better to pass in an S-Expression representing
;; the program to be compiled instead of strings, which are
;; clunky.

; Begin C functions definitions
(define square
  (compile *compile-state* 

    "square" ; name of the C function we want to extract 

    ; This is a simple C test function, it will square a number
    "#include <liblisp.h> /*liblisp needs to be installed and on include path*/
    cell *square(lisp *l, cell *args) { /*simple test function*/
          if(!cklen(args, 1) || !is_arith(car(args)))
                  RECOVER(l, \"\\\"expected (number)\\\" '%S \", args);
          if(is_float(car(args)))
              return mkfloat(l, floatval(car(args)) * floatval(car(args)));
          else
              return mkint(l, intval(car(args)) * intval(car(args)));
    }"))
;(define a
;  (compile *compile-state*
;    "a"
;    "#include <liblisp.h>
;    cell *a(lisp *l, cell *args) {
;            return mkfloat(l, 3.1);
;    }
;    "))