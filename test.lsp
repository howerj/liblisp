#!./lisp init.lsp
(define test 
  (lambda
    (compare expr result)
    (if 
      (compare expr result)
      t
      (begin 
        (put "Test failed: ")
        (print expr)
        (put " != ")
        (print result)
        (put "\n")
        (exit)))))

# Test suite
(begin
        (test = (factorial 6) 720)
        (test = (match 'abc 'abc) t)
        (test = (match 'a*c 'abbbc) t)
        (test = (match 'a*c 'ac) t)
        (test = (match 'a?c 'abc) t)
        (test = (match 'abc* 'abcd) t)
        (test = (match 'abc 'abcd) nil)
        (test = (match 'abcd 'abc) nil)
        (test = (cdr (assoc 'x '((x . a) (y . b)))) 'a)
        (test = (eval 'x '((x a) (y b))) '(a))
        "All tests passed")

(subst 'm 'b '(a b (a b c) d))
(list 'a 'b 'c)
(pair '(x y z) '(a b c))

# Example code

 (define frandom (lambda () (cabs (/ (coerce *float* (random)) *random-max*))))
 (define sum-of-squares (lambda (x y) (+ (cpow x 2) (cpow y 2))))
 
 (define monte-inner
   (lambda ()
     (if (<= (sum-of-squares (frandom) (frandom)) 1)
       1
       0)))
 
 (define monte-outer
   (lambda (iter c)
     (if (<= iter 0)
       c
       (monte-outer (- iter 1) (+ (monte-inner) c)))))
 
 (define monte-carlo-pi
   (lambda (iter)
     (* (/ (coerce *float* (monte-outer iter 0)) iter) 4)))

# (monte-carlo-pi 40)

