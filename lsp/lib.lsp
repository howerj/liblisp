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


;Being passed the wrong environment? Or evaluating at the wrong time?
;Reason for internal symbols for "if". Need progn? Eval all if into
;symbols, return and execute. Big changes to system needed."

(define !
  (lambda (x)
    (if (= x 1)
        1
        (* x (! (- x 1))))))

; pretty printing test
(quote 
 (1 2 3 
  (1 2 3 
   (1 2 3 "hello" 
    (1 2))) 
  4 5))
