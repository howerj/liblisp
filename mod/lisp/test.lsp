; This is a simple test suite, far more tests should be added here
; to fully test all aspects of the interpreter. Every single primitive
; should be tested here to both document functionality and make the
; test suite as comprehensive as possible.

; random floating point number between zero and one
(define frandom (lambda () (fabs (/ (coerce *float* (random)) *random-max*))))
(define sum-of-squares (lambda (x y) (+ (* x x) (* y y))))

; determine the value of pi with Monte-Carlo methods,
; more iterations creates a better approximation
(define monte-carlo-pi
  (lambda (iter)
    (let
        (inner ; hit?
          (lambda ()
            (if (<= (sum-of-squares (frandom) (frandom)) 1)
              1
              0)))
        (outer ; loop "iter" times
           (lambda (iter)
             (let 
               (i iter)
               (c 0)
               (progn
                 (set! i (- i 1))
                 (set! c (+ (inner) c))
                 (if 
                   (<= i 0)
                   (return c)
                   loop)
                 ()))))
    (* (/ (coerce *float* (outer iter)) iter) 4))))

; This is a series of simple tests that is not comprehensive
; at the moment.
(let
  (test (lambda
    (compare expr result)
    (if 
      (compare expr result)
      t
      (progn
        (format *output* "Test failed: %S != %S\n" expr result)
        (exit)))))
  (progn
    (test = (let (a 3) (b -4) (+ a b)) -1)
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
    (test = (regex "a?b" "ab")      t)
    (test = (regex "a?b" "b")       t)
    (test = (regex "a?b" "a")       nil)
    (test = (regex "^a" "a")        t)
    (test = (regex "^a" "ab")       t)
    (test = (regex "^a" "ba")       nil)
    (test = (ilog2 0)               0)
    (test = (ilog2 1)               0)
    (test = (ilog2 5)               2)
    (test = (ilog2 8)               3)
    (test = (substring "hello, world" 2 12) "llo, world")
    (test 
      (lambda 
          (tst pat) 
          (match pat tst))
        (coerce *string* 1.0) 
         "1\\.0*")
    (test = (cdr (assoc 'x '((x . a) (y . b)))) 'a)
    (test = (eval 'x '((x a) (y b))) '(a))
    (test equal (pair '(x y z) '(a b c)) '((x a) (y b) (z c)))
    (test equal (list 'a 'b 'c) '(a b c))
    (test equal (subst 'm 'b '(a b (a b c) d)) '(a m (a m c) d))
    ; module tests
    (if
      *have-line* 
      (progn
        (test = (line-editor-mode t) t)
        (test = (clear-screen) t)
        (clear-screen)
         t)
      nil)
    (put *output* "Self-Test Passed\n")
    t))

