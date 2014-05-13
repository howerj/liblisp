(define cadr (lambda (x) (car (cdr x))))
(define caddr (lambda (x) (car (cdr (cdr x)))))
(define cadddr (lambda (x) (car (cdr (cdr (cdr x))))))

(define sumofsqrs (lambda (x y) (+ (* x x) (* y y))))

(define gcd 
 (lambda (x y) 
  (if 
   (= 0 y) 
   x 
   (gcd y 
    (mod x y)))))


"being passed the wrong environment? Or evaluating at the wrong time?

Reason for internal symbols for \"if\". Need progn? Eval all if into
symbols, return and execute. Big changes to system needed.

"
(define !
  (lambda (N)
    (if (= N 1)
        1
        (* N (! (- N 1))))))

"doesn't work; example of the above"
(define tst (lambda (X) (if X (* X X) X)))
