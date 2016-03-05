;;; load shared objects or DLLs ;;;
;; depends on init.lsp ;;
;; @todo instead of just loading modules here, I should code modules
;; that load their DLL, perform tests, etc.

; This should take a module path as well as name as an argument
(define load-lisp-module ; load a module {.so or .dll depending on the operating system}
  (compile 
    "load a compiled module (a shared object or DLL)"   
    (name)
    (if 
      (and 
        *have-dynamic-loader* 
        (= (dynamic-load-lisp-module (string->symbol (join "" (list "liblisp_" name (if (= *os* "unix") ".so" ".dll"))))) 'error))
         (progn
	   (format *output* "module: %s %s\n" name (dynamic-error))
	   (define-eval (string->symbol (join "" (list "*have-" name "*"))) nil))
         (define-eval (string->symbol (join "" (list "*have-" name "*"))) t))))

(progn ; load all known modules
  ; I should probably rename the modules, or provide alternate names for them,
  ; like "sql" should really be "sqlite3"
 (load-lisp-module "bignum") ; bignum module
 (load-lisp-module "crc")    ; crc module
 (load-lisp-module "line")   ; line editing library and module
 (load-lisp-module "math")   ; math module
 (load-lisp-module "text")   ; diff, more string handling and tsort module
 (load-lisp-module "unix")   ; unix interface module
 (load-lisp-module "x11")    ; x11 window module
 (load-lisp-module "sql")    ; sql interface 
 (load-lisp-module "tcc")    ; tiny c compiler (leaks memory, if used)
 (load-lisp-module "xml")    ; XML parser and writer
 (load-lisp-module "curl")   ; Curl library
 t)

