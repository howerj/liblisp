#!./lisp 
; Initialization code
;
; The job of the initialization code is to define enough functionality to
; sensibly load modules and other lisp files.
;
; For the entire project:
; @todo Each function should have much longer documentation strings
; @todo Each function should be limited to the scope that it is needed
; @todo Each function should have a series of tests for it
;

(progn
        (define list 
          (lambda x x))

        (define join
          (compile 
            "join a list of strings" 
            (sep l)
            (foldl 
              (lambda 
                (_join1 _join2) 
                (scons _join1 (scons sep _join2)))
              reverse.l)))
        
        (define and ; @bug Incorrect, evaluates all args
          (compile 
            "return t if both arguments evaluate to true, nil otherwise" 
            (x y) 
                  (if x 
                    (if y t nil) nil)))

        (define is-nil       
          (compile 
            "is x nil?" 
            (x) 
            (if x nil t)))

        (define or ; @bug Incorrect, evaluates all args
          (compile 
            "return r if either arguments evaluate to true" 
            (x y) 
            (cond 
              (x t) 
              (y t) 
              (t nil))))

        (define is-type   
          (compile 
            "is a object of a certain type" 
            (type-enum x) 
            (eq type-enum 
                type-of.x)))

        (define is-symbol 
          (compile 
            "is x a symbol" 
            (x) 
            (is-type *symbol* x)))

        (define is-string 
          (compile 
            "is x a string" 
            (x) 
            (is-type *string* x)))

        (define exit    
          (compile 
            "exit the interpreter" 
            ()  
            (signal *sig-term*)))

        (define string->symbol 
          (compile 
            "coerce a symbol into a string" 
            (sym) 
            (coerce *symbol* sym)))

        (define *file-separator* 
          (cond 
            ((= *os* "unix") "/")
            ((= *os* "windows") "\\")
            (t "/")))

        (define make-path
          (compile 
            "make a path from a list of directories and a file" 
            (l)
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
                 (is-eof.S nil)
                 ((eq (setq x read.S) 'error) (on-error-fn S file))
                 (t (progn
                      (setq x eval.x)
                      (if print-result
                        (format *output* "%S\n" x)
                        nil)
                      t)))))
             (cond ; If we have been given a potential file name, try to open it
              ((or is-string.file is-symbol.file)
                (let 
                  (file-handle 
                    (open *file-in* file))
                  (if 
                    is-input.file-handle
                    (progn 
                      (while 
                        (eval-file-inner file-handle on-error-fn))
                      (close file-handle) ; Close file!
                      nil)
                    (progn 
                      (format *error* "Could not open file for reading: %s\n" file)
                           'error))))
              (is-input.file ; We must have been passed an input file port
               (eval-file-inner file on-error-fn)) ; Do not close file!
              (t ; User error
                (progn
                  (put *error* "(error \"Not a file name or a output IO type\" %S)\n" file) 
                  'error))))))
        (let
         (exit-if-not-eof
          (lambda 
            (in file) 
           (if is-eof.in
            t
            (progn 
              (format 
                *error* 
                "(error \"eval-file failed\" %S %S)\n" in file) 
              (exit)))))

         (error-ignore
           (lambda 
             (in file)
             t
             nil))

         (progn
          (eval-file (make-path '("lsp" "mods.lsp")) exit-if-not-eof nil)
          (eval-file (make-path '("lsp" "base.lsp")) exit-if-not-eof nil)
          (eval-file (make-path '("lsp" "data.lsp")) exit-if-not-eof nil)
          (eval-file (make-path '("lsp" "sets.lsp")) exit-if-not-eof nil)
          (eval-file (make-path '("lsp" "symb.lsp")) exit-if-not-eof nil)
          (eval-file (make-path '("lsp" "test.lsp")) exit-if-not-eof nil)
          (eval-file (make-path '("lsp" "sql.lsp"))  exit-if-not-eof nil)
        ; (eval-file (make-path '("lsp" "tcc.lsp"))  exit-if-not-eof nil)
        
          (define *lisprc* ".lisprc") ; start up file
          (define *home* nil)

          (cond
            ((setq *home* (get-system-variable "LISPHOME")) *home*) ; highest priority
            ((setq *home* (get-system-variable "HOME"))     *home*) ; unix
            ((setq *home* (get-system-variable "HOMEPATH")) *home*) ; windows
            ((setq *home* nil) *home*))
          (if *home*
            (eval-file 
              (make-path 
                (list *home* *lisprc*)) error-ignore nil)
            nil)
         'done))
 'ok)

