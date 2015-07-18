#!../bin/lisp
(define cadr (lambda (x) (car (cdr x))))
(define caddr (lambda (x) (car (cdr (cdr x)))))
(define cadddr (lambda (x) (car (cdr (cdr (cdr x))))))

(define gcd 
 (lambda (x y) 
  (if 
   (= 0 y) 
   x 
   (gcd y 
    (mod x y)))))

(define bool
 (lambda
   (x)
   (if
    (eqt x ())
    0
    (if
     (eqt x t)
     1
     nil)))) # error instead of nil?

#Being passed the wrong environment? Or evaluating at the wrong time?
#Reason for internal symbols for "if". Need progn? Eval all if into
#symbols, return and execute. Big changes to system needed."

(define !
  (lambda (x)
    (if (= x 1)
        1
        (* x (! (- x 1))))))


# This show cases the inefficiency of my environment!
(define explode
 (lambda (x y)
  (if (scdr x)
   (explode (scdr x) (cons (scar x) y))
   (reverse (cons (scar x) y)))))

# pretty printing test
#(quote 
# (1 2 3 
#  (1 2 3 
#   (1 2 3 "hello" 
#    (1 2))) 
#  4 5))
