#!./lisp 
; Initialization code
;
; The job of the initialization code is to define enough functionality to
; sensibly load modules.
;
; type checking information
;
; @todo file name escaping "a b" ==> "a\ b"
; @todo more utility functions
; @todo tabs for formating

(progn
        ; @bug Incorrect, evaluates all args
        (define and     (lambda (x y) (if x (if y t nil) nil)))
        ; @bug Incorrect, evaluates all args
        (define or      (lambda (x y) (cond (x t) (y t) (t nil))))
        (define type?   (lambda (type-enum x) (eq type-enum (type-of x))))
        (define symbol? (lambda (x) (type? *symbol* x)))
        (define string? (lambda (x) (type? *string* x)))
        (define exit    (lambda ()  (raise-signal *sig-term*)))
        (define string->symbol (lambda (sym) (coerce *symbol* sym)))
        (define *file-separator* 
                (cond 
        	        ((= *os* "unix") "/")
		        ((= *os* "windows") "\\")
                        (t "/")))
        (define make-path
                (lambda	(l)
                        (join *file-separator* l)))
 'ok)

; Evaluate an entire file, stopping on 'error
; (eval-file string-or-input-port)
(define eval-file
  (lambda (file on-error-fn print-result)
    (let 
     (x ())
     (eval-file-inner ; evaluate all the expressions in a file 
      (lambda (S on-error-fn)
       (cond
         ((eof? S) nil)
         ((eq (set! x (read S)) 'error) (on-error-fn S file))
         (t (progn
              (set! x (eval x))
              (if print-result
                (format *output* "%S\n" x)
                nil)
              loop)))))
     (cond ; If we have been given a potential file name, try to open it
      ((or (string? file) (symbol? file))
        (let (file-handle (open *file-in* file))
          (if (input? file-handle)
            (progn (eval-file-inner file-handle on-error-fn)
                   (close file-handle) ; Close file!
                   nil)
            (progn (format *error* "Could not open file for reading: %s\n" file)
                   'error))))
      ((input? file) ; We must have been passed an input file port
       (eval-file-inner file on-error-fn)) ; Do not close file!
      (t ; User error
        (progn
          (put *error* "(error \"Not a file name or a output IO type\" %S)\n" file) 
          'error))))))

(let
 (exit-if-not-eof
  (lambda (in file) 
   (if (eof? in)
    t
    (progn (format *error* "(error \"eval-file failed\" %S %S)\n" in file) (exit)))))
 (progn
  (eval-file (make-path '(mod lsp mods.lsp)) exit-if-not-eof nil)
  (eval-file (make-path '(mod lsp base.lsp)) exit-if-not-eof nil)
  (eval-file (make-path '(mod lsp data.lsp)) exit-if-not-eof nil)
  (eval-file (make-path '(mod lsp sets.lsp)) exit-if-not-eof nil)
  (eval-file (make-path '(mod lsp symb.lsp)) exit-if-not-eof nil)
  (eval-file (make-path '(mod lsp test.lsp)) exit-if-not-eof nil)
 'ok))

