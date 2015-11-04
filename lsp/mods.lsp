;;; load shared objects or DLLs ;;;
;; depends on init.lsp ;;

(define load-module ; load a module {.so or .dll depending on the operating system}
  (compile 
    "load a compiled module (a shared object or DLL)"   
    (name)
    (if 
      (and 
        *have-dynamic-loader* 
        (= (dynamic-open (string->symbol (join "" (list "liblisp_" name (if (= *os* "unix") ".so" ".dll"))))) 'error))
         (define-eval (string->symbol (join "" (list "*have-" name "*"))) nil)
         (define-eval (string->symbol (join "" (list "*have-" name "*"))) t))))

(progn ; load all known modules
 (load-module "bignum") ; bignum module
 (load-module "crc")    ; crc module
 (load-module "line")   ; line editing library and module
 (load-module "math")   ; math module
;(load-module "sql")    ; sql interface (leaks memory)
;(load-module "tcc")    ; tiny c compiler (leaks memory)
 (load-module "text")   ; diff, more string handling and tsort module
 (load-module "unix")   ; unix interface module
 (load-module "x11")    ; x11 window module
 t)

