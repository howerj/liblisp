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

(define sumofsqrs (lambda (x y) (+ (* x x) (* y y))))

(define !
  (lambda (N)
    (if (= N 1)
        1
        (* N (! (- N 1))))))


