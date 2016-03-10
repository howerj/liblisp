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
	(nil? (= (dynamic-load-lisp-module 
	     (string->symbol 
	       (join "" 
		     (list "liblisp_" name 
			   (if 
			     (= *os* "unix") 
			     ".so" 
			     ".dll"))))) 
	   'error)))
	(define-eval (string->symbol (join "" (list "*have-" name "*"))) t)
      (define-eval (string->symbol (join "" (list "*have-" name "*"))) nil))))

(progn ; load all known modules
 (load-lisp-module "base")   ; basic liblisp system library
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
 (load-lisp-module "pcre")   ; Perl-Compatible regular expressions
 t)

