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

(define a (quote (1 2 3)))
(define b a)
a
b
(set b 2)

