#!./lisp 
; Initialization code
;
; These functions come from various sources, such as "The Roots Of Lisp"
; essay, the book "The little schemer" and fragments from all over.

; To-do
; * unify, prolog interpreter, statistics package, symbolic manipulation
;   of sets, logic and state machine minimization routines and all kinds
;   off neat stuff.
;

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

(define nil? (lambda (x) (if x nil t)))

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
  (lambda (file on-error-fn)
    ; This bit does the evaluation of a file input object
    (letrec 
      (eval-file-inner
       (lambda (S on-error-fn)
        (if (eof? S) 
          nil
          (let* 
            (x (read S))
            (if (eq x 'error) 
              (on-error-fn S)
              (begin 
                (print (eval x))
                (put "\n")
                (eval-file-inner S on-error-fn)))))))
    (cond 
      ; If we have been given a potential file name, try to open it
      ((or (string? file) (symbol? file))
        (let* (file-handle (open *file-in* file))
          (if (input? file-handle)
            (begin (eval-file-inner file-handle on-error-fn)
                   (close file-handle) ; Close file!
                   nil)
            (begin (put "could not open file for reading\n")
                   'error))))
      ; We must have been passed an input file port
      ((input? file)
       (eval-file-inner file on-error-fn)) ; Do not close file!
      ; User error
      (t 
        (begin 
          (put "Not a file name or a output IO type\n") 
          'error))))))

; exit the interpreter
(define exit (lambda () (error -1)))

(define exit-if-not-eof 
  (lambda (in) 
    (if 
      (not (eof? in))
      (exit)
      nil)))

(if *have-dynamic-loader*
  (begin
    (if (not (= (dynamic-open "liblisp_os.so") 'error))
      (define *have-os* t)
      (define *have-os* nil))
    (if (not (= (dynamic-open "liblisp_sql.so") 'error))
      (define *have-sql* t)
      (define *have-sql* nil))
    (if (not (= (dynamic-open "liblisp_tcc.so") 'error))
      (define *have-tcc* t)
      (define *have-tcc* nil))
    (if (not (= (dynamic-open "liblisp_crc.so") 'error))
      (define *have-crc* t)
      (define *have-crc* nil))
    (if (not (= (dynamic-open "liblisp_line.so") 'error))
      (define *have-line* t)
      (define *have-line* nil))
    (if (not (= (dynamic-open "liblisp_math.so") 'error))
      (define *have-math* t)
      (define *have-math* nil))
    t) nil)

; Evaluate a series of "modules", they are just files with defines in them,
; a proper module system nor ways of representing dependencies between them
; has been devised yet. They must be executed in order.
(eval-file 'base.lsp exit-if-not-eof)
(eval-file 'sets.lsp exit-if-not-eof)
(eval-file 'symb.lsp exit-if-not-eof)
(eval-file 'test.lsp exit-if-not-eof)

;(if *have-tcc* (eval-file "mod/c.lsp" exit-if-not-eof) nil)

