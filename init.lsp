#!./lisp 
; Initialization code
;
; The job of the initialization code is to define enough functionality to
; sensibly load modules.
;
; type checking information

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
(define arithmetic?(lambda (x) (or (integer? x) (float? x))))
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


; Evaluate an entire file, stopping on 'error
; (eval-file STRING)
; (eval-file INPUT-PORT)
(define eval-file
  (lambda (file on-error-fn)
    ; This bit does the evaluation of a file input object
    (let 
      (eval-file-inner
       (lambda (S on-error-fn)
        (if (eof? S) 
          nil
          (let 
            (x (read S))
            (if (eq x 'error) 
              (on-error-fn S)
              (progn
                ; (format *output* "%S\n" (eval x))
                (print *output* (eval x))
                (put *output* "\n")
                loop))))))
    (cond 
      ; If we have been given a potential file name, try to open it
      ((or (string? file) (symbol? file))
        (let (file-handle (open *file-in* file))
          (if (input? file-handle)
            (progn (eval-file-inner file-handle on-error-fn)
                   (close file-handle) ; Close file!
                   nil)
            (progn (put *error* "could not open file for reading\n")
                   'error))))
      ; We must have been passed an input file port
      ((input? file)
       (eval-file-inner file on-error-fn)) ; Do not close file!
      ; User error
      (t 
        (progn
          (put *error* "Not a file name or a output IO type\n") 
          'error))))))

; exit the interpreter
(define exit (lambda () (raise *sig-term*)))

(define exit-if-not-eof 
  (lambda (in) 
    (if 
      (not (eof? in))
      (exit)
      nil)))

(define string-to-symbol 
  (lambda (sym)
    (coerce *symbol* sym)))

(define load-module
  (lambda (name)
    (if 
      (and 
        *have-dynamic-loader* 
        (not (= (dynamic-open (string-to-symbol (join "" "liblisp_" name (if (= os "unix") ".so" ".dll")))) 'error)))
      (define-eval (string-to-symbol (join "" "*have-" name "*")) t)
      (define-eval (string-to-symbol (join "" "*have-" name "*")) nil))))

; Evaluate a series of "modules", they are just files with defines in them,
; a proper module system nor ways of representing dependencies between them
; has been devised yet. They must be executed in order.
 (eval-file 'base.lsp exit-if-not-eof)
;(eval-file 'mod/sets.lsp exit-if-not-eof)
 (eval-file 'mod/symb.lsp exit-if-not-eof)

(progn
 (load-module "unix")   ; unix interface module
 (load-module "sql")    ; sql interface
 (load-module "tcc")    ; tiny c compiler
 (load-module "crc")    ; crc module
 (load-module "line")   ; line editing library and module
 (load-module "math")   ; math module
 (load-module "x11")    ; x11 window module
 (load-module "diff")   ; diff module
 (load-module "tsort")  ; tsort module
 (load-module "bignum") ; bignum module
 t)

(eval-file 'test.lsp exit-if-not-eof)
