"(3 4):" (cdr (quote (2 3 4)))
"(4):"   (cdr (cdr (quote (2 3 4))))
"():"    (cdr (cdr (cdr (quote (2 3 4)))))
"9:"     (add 2 3 4)

"4:"  (begin 1 2 3 4)
"3:"  (begin (if () 2 3))
"-1:" (sub 4 5)
"():" ()
"():" nil
"#t:" t

"(1 2 3):" (define a (quote (1 2 3)))
"(1 2 3):" (define b a)
a
b
(set b 2)
"#PRIMOP:" (define + add)
"5" (+ 2 3)
"Need to sort this out!" (define myif if) 

"(define test (lambda (x) (+ x 1))):"
(define test (lambda (x) (+ x 1)))
(define test2 (lambda (x) (+ (test x) 1)))

