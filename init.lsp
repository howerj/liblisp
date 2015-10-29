#!./lisp 
; Initialization code
;
; The job of the initialization code is to define enough functionality to
; sensibly load modules and other lisp files.
;
; @todo The doc-strings could be made a lot longer, giving examples on
;       their use.
; @note I could use tabs for formatting
;

(progn
        (define join
                (compile "join a list of strings" (sep l)
                        (foldl 
                        (lambda (a b) (scons a (scons sep b)))
                        (reverse l))))
        ; @bug Incorrect, evaluates all args
        (define and     
                (compile 
                  "return t if both arguments evaluate to true, nil otherwise" 
                  (x y) 
                  (if x 
                    (if y t nil) nil)))
        ; @bug Incorrect, evaluates all args
        (define or      (compile "return r if either arguments evaluate to true" (x y) (cond (x t) (y t) (t nil))))
        (define type?   (compile "is a object of a certain type" (type-enum x) (eq type-enum (type-of x))))
        (define symbol? (compile "is x a symbol" (x) (type? *symbol* x)))
        (define string? (compile "is x a string" (x) (type? *string* x)))
        (define exit    (compile "exit the interpreter" ()  (raise-signal *sig-term*)))
        (define string->symbol (compile "coerce a symbol into a string" (sym) (coerce *symbol* sym)))
        (define *file-separator* 
                (cond 
        	        ((= *os* "unix") "/")
		        ((= *os* "windows") "\\")
                        (t "/")))
        (define make-path
                (compile "make a path from a list of directories and a file" (l)
                        (join *file-separator* l)))
        (define eval-file
          (lambda 
            "evaluate a file given an input port or a string"
            (file on-error-fn print-result)
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
          (eval-file (make-path '("lsp" "mods.lsp")) exit-if-not-eof nil)
          (eval-file (make-path '("lsp" "base.lsp")) exit-if-not-eof nil)
          (eval-file (make-path '("lsp" "data.lsp")) exit-if-not-eof nil)
          (eval-file (make-path '("lsp" "sets.lsp")) exit-if-not-eof nil)
          (eval-file (make-path '("lsp" "symb.lsp")) exit-if-not-eof nil)
          (eval-file (make-path '("lsp" "test.lsp")) exit-if-not-eof nil)
        ; (eval-file (make-path '("lsp" "ltcc.lsp")) exit-if-not-eof nil) ; requires liblisp_tcc.so
         'done))
 'ok)

