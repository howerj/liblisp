; Differentiation of polynomials and symbolic simplification
; From http://shrager.org/llisp/, which is the tutorial for
; an old lisp called "P-LISP" which ran on the Apple II.
;
; The exact links into the tutorial is:
;       <http://shrager.org/llisp/23.html>
;       <http://shrager.org/llisp/24.html>
;
; The functions there have been translated and extended.

; The symbolic differentiation function uses these rules:
; D[0,x]=0
; D[x,x]=1
; D[(u+v),x]=D[u,x]+D[v,x]
; D[(u-v),x]=D[u,x]-D[v,x]
; D[(uv),x]=uD[v,x]+vD[u,x]
; D[(u^n),x]=nu^(n-1)D[u,x]
 
; helper functions
(define function    (lambda (poly) (car poly)))
(define first-term  (lambda (poly) (cadr poly)))
(define second-term (lambda (poly) (caddr poly)))

(define derive
  (lambda (poly var)
    (cond
      ((atom? poly) (derv-atom poly var))
      ((eq '+ (function poly))
       (derv-sum poly var))
      ((eq '- (function poly))
       (derv-minus poly var))
      ((eq '* (function poly))
       (derv-prod poly var))
      ((eq 'pow (function poly))
       (derv-pow poly var)))))

(define derv-atom
  (lambda (poly var)
    (cond
      ((eq poly var) 1)
      (t 0))))

(define derv-sum
  (lambda (poly var)
    (list '+
          (derive (first-term poly) var)
          (derive (second-term poly) var))))

(define derv-minus
  (lambda (poly var)
    (list '-
          (derive (first-term poly) var)
          (derive (second-term poly) var))))

(define derv-prod
  (lambda (poly var)
    (list '+
          (list '*
                (first-term poly)
                (derive (second-term poly) var))
          (list '*
                (second-term poly)
                (derive (first-term poly) var)))))

(define derv-pow
  (lambda (poly var)
    (list '*
          (list '*
                (second-term poly)
                (list 'pow
                      (first-term poly)
                      (- (second-term poly) 1)))
                (derive (first-term poly) var))))

; The simplification function uses these rules, where
; 'a' and 'b' are two constants.
;
;    Lisp form  Simplified form
;    ---------  ---------------
;    *   ? 0          0
;    *   0 ?          0
;    *   1 ?          ?
;    *   ? 1          ?
;    *   a b         a*b
;    +   0 ?          ?
;    +   ? 0          ?
;    +   a b         a+b
;    -   ? 0          ?
;    -   ? 0         a-b
;    -   a a          0
;    /   a a          1
;    /   a 1          a
;    /   a 0        error
;    pow ? 0          1
;    pow ? 1          ?
;    pow 0 ?          0
;    pow 1 ?          1
;
;    x is a symbol
;
;    @todo More operations can be added
;
;    &   0 ?          0
;    &   ? 0          0
;    &   a b         a&b
;    &   x x          x
;    |   0 ?          ?
;    |   ? 0          ?
;    |   x x          x
;    |   a b         a|b
;    ^   ? 0          ?
;    ^   0 ?          ?
;    ^   a b         a^b
;    ^   x x          0
;    sin pi           0
;    cos pi          -1
;    ...
;
;    More simplifications can be done if:
;    * simplify can peek into expressions before hand, such
;      as:
;       (log (exp x)) -> x
;       (exp (log x)) -> x
;    * symbols can be resolved:
;         x <= 3
;         (simplify '(+ (* x x) y) environment) => (+ 9 y)
;      or even just to allow function replacement
;         & := <SUBR:4215008>
;         (simplify '(& x x)) => (<SUBR:4215008> x x)
;    * unknown function symbols have their arguments simplified
;         (define square (lambda (x) (* x x)))
;         (simplify '(square (+ 2 2)))
;      -> (square 4)
;
;    This would be the start of an optimizing compiler.
;

(define constant-term?
  (lambda (poly) 
    (and 
      (arithmetic? (first-term poly)) 
      (arithmetic? (second-term poly)))))

; (define operations '((* t) (+ t) (- t) (/ t) (pow t)))

(define simplify1 (lambda (poly)
  (cond
       ((atom? poly) poly)
       ((eq '/ (function poly))
        (cond
          ((eq 0 (second-term poly)) error)
          ((constant-term? poly) 
            (/ (first-term poly) (second-term poly)))
          ((eq 1 (second-term poly)) (first-term poly))
          ((eq (first-term poly) (second-term poly)) 1)
          (t (list '/
              (simplify1 (first-term poly))
              (simplify1 (second-term poly))))))
       ((eq '* (function poly))
         (cond
           ((constant-term? poly) 
            (* (first-term poly) (second-term poly)))
           ((eq 0 (first-term poly)) 0)
           ((eq 0 (second-term poly)) 0)
           ((eq 1 (first-term poly))
            (simplify1 (second-term poly)))
           ((eq 1 (second-term poly))
            (simplify1 (first-term poly)))
           (t (list '*
               (simplify1 (first-term poly))
               (simplify1 (second-term poly))))))
        ((eq '+ (function poly))
         (cond 
           ((constant-term? poly) 
            (+ (first-term poly) (second-term poly)))
           ((eq 0 (first-term poly))
            (simplify1 (second-term poly)))
           ((eq 0 (second-term poly))
            (simplify1 (first-term poly)))
           (t (list '+
               (simplify1 (first-term poly))
               (simplify1 (second-term poly))))))
        ((eq '- (function poly))
          (cond
            ((constant-term? poly) 
             (- (first-term poly) (second-term poly)))
            ((eq (first-term poly) (second-term poly)) 0)
            ((eq 0 (second-term poly))
             (simplify1 (first-term poly)))
            (t (list '-
                (simplify1 (first-term  poly))
                (simplify1 (second-term poly))))))
        ((eq 'pow (function poly))
          (cond
            ((constant-term? poly)
             (pow (first-term poly) (second-term poly)))
            ((eq 0 (second-term poly)) 1)
            ((eq 1 (second-term poly))
             (simplify1 (first-term poly)))
            ((eq 0 (first-term poly)) 0)
            ((eq 1 (first-term poly)) 1) ))
       (t poly))))

; Repeatedly apply simplify1 until there is no change
(define simplify
  (lambda (poly)
    (let (poly1 ()) 
      (progn
        (set! poly1 (simplify1 poly))
        (if (equal poly1 poly) 
          (return poly)
          (set! poly poly1))
          loop
          error))))

; turn infix (2 + 2) to prefix (+ 2 2)
(define infix->prefix
  (lambda (poly)
    (cond
      ((list? poly) 
       (list 
         (cadr poly)
         (infix->prefix (car poly))
         (infix->prefix (caddr poly))))
      (t poly))))

; turn prefix (+ 2 2) to infix (2 + 2)
(define prefix->infix
  (lambda (poly)
    (cond
    ((list? poly)
     (list
        (prefix->infix (cadr  poly))
        (car poly)
        (prefix->infix (caddr poly))))
    (t poly))))

