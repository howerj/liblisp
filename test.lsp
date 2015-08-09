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

(begin
        (test = (let* (a 3) (b -4) (+ a b)) -1)
        (test = (if 'a 'b 'c)          'b)
        (test = (if  0 'b 'c)          'b)
        (test = (if () 'b 'c)          'c)
        (test = (cond)                  nil)
        (test = (cond (nil 1) (t 2))    2)
        (test = (factorial 6)           720)
        (test = (match 'abc  'abc)      t)
        (test = (match 'a*c  'abbbc)    t)
        (test = (match 'a*c  'ac)       t)
        (test = (match 'a.c  'abc)      t)
        (test = (match 'abc* 'abcd)     t)
        (test = (match 'abc  'abcd)     nil)
        (test = (match 'abcd 'abc)      nil)
        (test (lambda 
                (tst pat) 
                (match pat tst)) 
              (coerce *string* 1.0) 
              "1\\.0*")
        (test float-equal (cos (/ pi 3)) 0.5)
        (test = (cdr (assoc 'x '((x . a) (y . b)))) 'a)
        (test = (eval 'x '((x a) (y b))) '(a))
        (put "Self-Test Passed\n")
        t)

# Test suite

(if
  *have-line* 
  (begin 
    (test = (line-editor-mode t) t)
    (test = (clear-screen) t)
    (put "line editor test passed")
    (clear-screen)
    t)
  nil)

(if *have-line* (clear-screen) t)

