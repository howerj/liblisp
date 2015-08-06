#!./lisp init.lsp

(define frandom (lambda () (fabs (/ (coerce *float* (random)) *random-max*))))
(define sum-of-squares (lambda (x y) (+ (pow x 2) (pow y 2))))

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

(subst 'm 'b '(a b (a b c) d))
(list 'a 'b 'c)
(pair '(x y z) '(a b c))

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
        (test float-equal (cos (/ pi 3)) 0.5)
        (test float-equal (monte-carlo-pi 40) 3.1) # PRNG should provide the same sequence
        (test = (cdr (assoc 'x '((x . a) (y . b)))) 'a)
        (test = (eval 'x '((x a) (y b))) '(a))
        (put "All tests passed.\n")
        t)

