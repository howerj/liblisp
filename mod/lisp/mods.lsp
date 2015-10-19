;;; load shared objects or DLLs ;;;
;; depends on init.lsp ;;

(define load-module ; load a module {.so or .dll depending on the operating system}
  (lambda (name)
    (if 
      (and 
        *have-dynamic-loader* 
        (= (dynamic-open (string->symbol (join "" "liblisp_" name (if (= *os* "unix") ".so" ".dll")))) 'error))
         (define-eval (string->symbol (join "" "*have-" name "*")) nil)
         (define-eval (string->symbol (join "" "*have-" name "*")) t))))

(progn ; load all known modules
 (load-module "unix")   ; unix interface module
;(load-module "sql")    ; sql interface (leaks memory)
;(load-module "tcc")    ; tiny c compiler (leaks memory)
 (load-module "crc")    ; crc module
 (load-module "line")   ; line editing library and module
 (load-module "math")   ; math module
 (load-module "x11")    ; x11 window module
 (load-module "diff")   ; diff module
 (load-module "tsort")  ; tsort module
 (load-module "bignum") ; bignum module
 t)

