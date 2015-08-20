#!./lisp 
; Initialization code
;
; These functions come from various sources, such as "The Roots Of Lisp"
; essay, the book "The little schemer" and fragments from all over.

; Todo
; * list equality
; * list structural equality
; * unify, prolog interpreter

; doesn't work correctly yet
(define assert
  (flambda (test)
    (let* (te (begin (print (env)) (eval (car test) (env))))
      (if te
        nil
        (begin (put "Assertion failed:\n\t")
               (print (car test))
               (put "\n")
               (throw -1))))))

(define type?      (lambda (type-enum x) (eq type-enum (type-of x))))
(define list?      (lambda (x) (type? *cons* x)))
(define atom?      (lambda (x) (if (list? x) nil t)))
(define float?     (lambda (x) (type? *float* x)))
(define integer?   (lambda (x) (type? *integer* x)))
(define symbol?    (lambda (x) (type? *symbol* x)))
(define string?    (lambda (x) (type? *string* x)))
(define io?        (lambda (x) (type? *io* x)))
(define hash?      (lambda (x) (type? *hash* x)))
(define string?    (lambda (x) (type? *string* x)))
(define procedure? (lambda (x) (type? *procedure* x)))
(define primitive? (lambda (x) (type? *primitive* x)))
(define char?      (lambda (x) (and (string? x) (= (length x) 1))))
(define dotted?    (lambda (x) (and (list? x) (not (list? (cdr x))))))

(define null?
  (lambda (x)
    (cond
      ((string? x)  (eq x ""))
      ((float? x)   (eq x 0.0))
      ((integer? x) (eq x 0))
      ((hash? x)    (eq (coerce *cons* x) nil))
      (t (eq nil x)))))  

(define not
  (lambda (x)
    (null? x)))

; @bug Incorrect, evaluates all args
(define and
  (lambda (x y)
    (if x
      (if y t nil)
       nil)))

; @bug Incorrect, evaluates all args
(define or
  (lambda (x y)
    (cond (x t)
          (y t)
          (t nil))))

; Evaluate an entire file, stopping on 'error
; (eval-file STRING)
; (eval-file INPUT-PORT)
(define eval-file
  (lambda (file)
    ; This bit does the evaluation of a file input object
    (letrec 
      (eval-file-inner
       (lambda (S)
        (if (eof? S) 
          nil
          (let* 
            (x (read S))
            (if (eq x 'error) 
              nil
              (begin 
                (print (eval x))
                (put "\n")
                (eval-file-inner S)))))))
    (cond 
      ; If we have been given a potential file name, try to open it
      ((or (string? file) (symbol? file))
        (let* (file-handle (open *file-in* file))
          (if (input? file-handle)
            (begin (eval-file-inner file-handle)
                   (close file-handle) ; Close file!
                   nil)
            (begin (put "could not open file for reading\n")
                   'error))))
      ; We must have been passed an input file port
      ((input? file)
       (eval-file-inner file)) ; Do not close file!
      ; User error
      (t 
        (begin 
          (put "Not a file name or a output IO type\n") 
          'error))))))

; Evaluate a series of "modules", they are just files with defines in them,
; a proper module system nor ways of representing dependencies between them
; has been devised yet. They must be executed in order.
(eval-file 'base.lsp)
(eval-file 'sets.lsp)
(eval-file 'diff.lsp)
(eval-file 'test.lsp)

