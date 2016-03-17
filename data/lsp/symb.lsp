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
(define function    (lambda "get the function from a expression" (poly) (car poly)))
(define first-term  (lambda "get the first term of a expression" (poly) (cadr poly)))
(define second-term (lambda "get the second term of a expression" (poly) (caddr poly)))

(define derive
  (lambda "differentiate a expression" (poly var)
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
  (lambda "differentiate an atom" (poly var)
    (cond
      ((eq poly var) 1)
      (t 0))))

(define derv-sum
  (lambda "differentiate a sum" (poly var)
    (list '+
          (derive (first-term poly) var)
          (derive (second-term poly) var))))

(define derv-minus
  (lambda "differentiate a subtraction" (poly var)
    (list '-
          (derive (first-term poly) var)
          (derive (second-term poly) var))))

(define derv-prod
  (lambda "differentiate a product" (poly var)
    (list '+
          (list '*
                (first-term poly)
                (derive (second-term poly) var))
          (list '*
                (second-term poly)
                (derive (first-term poly) var)))))

(define derv-pow
  (lambda "differentiate an power term" (poly var)
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
;    exp 1            e
;    exp 0            1
;    log 1            0
;    log e            1
;
;    x is a symbol
;
;    @todo More operations can be added, and ones on operations
;          that take more than two (or only one) operands.
;
;    &   0 ?          0
;    &   ? 0          0
;    &   a b         a&b
;    &   x x          x
;    &   M x          x
;    &   x M          x
;    |   0 ?          ?
;    |   ? 0          ?
;    |   x x          x
;    |   M x          M
;    |   x M          M
;    |   a b         a|b
;    ^   ? 0          ?
;    ^   0 ?          ?
;    ^   a b         a^b
;    ^   x x          0
;    ^   M x         ~x
;    ^   x M         ~x
;    ~   x           ~x
;    sin pi           0
;    cos pi          -1
;    ...
;
;    'M' is all bits set
;
;    There are established rules for simplification of logic, but
;    doing it well and generally means changing the approach taken
;    here. However the general approach is NP hard.
;
;    Constant folding can also be applied to strings as well.
;
;    @warning special care should be taken with floating point
;             numbers, because the simplify function does not.
;

; @todo There should be an option to turn this off

(define constant-term?
  (lambda "is the arity-2 term composed of only constants?" (poly) 
    (and 
      (arithmetic? (first-term poly)) 
      (arithmetic? (second-term poly)))))

(define simplifyn
  (compile "simplify an arity-n expression"
    (poly)
    (let 
      (len (length poly))
      (cond
        ((= len 3) (simplify2 poly))
        ((= len 2) (simplify1 poly))
        ((= len 1) poly)
        ((= len 0) poly)
        (t (append
             (list (function poly))
             (map1 simplifyn (cdr poly))))))))

(define simplify1
  (lambda "simplify an arity-1 expression"
    (poly)
    (cond
      ((atom? poly) poly)
      ; @bug x-partially-equal only works when "?" is an atom 
      ((x-partially-equal '(log (exp ?)) poly) (simplifyn (cadadr poly)))
      ((x-partially-equal '(exp (log ?)) poly) (simplifyn (cadadr poly)))
      ((eq 'exp (function poly))
       (cond
         ((eq 0 (first-term poly)) 1)
         ((eq 1 (first-term poly)) 'e)
         (t (list 'exp (simplifyn (first-term poly))))))
      ((eq 'log (function poly))
       (cond
         ((eq  1 (first-term poly)) 0)
         ((eq 'e (first-term poly)) 1)
         (t (list 'log (simplifyn (first-term poly))))))
      (t (list (function poly)
               (simplifyn (first-term poly)))))))

(define *all-bits-set* -1)

; @todo error propagation (any-function error ?) or (any-function ? error) => error
(define simplify2 
  (lambda "simplify an arity-2 expression" 
    (poly)
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
                (simplifyn (first-term poly))
                (simplifyn (second-term poly))))))
         ((eq '* (function poly))
           (cond
             ((constant-term? poly) 
              (* (first-term poly) (second-term poly)))
             ((eq 0 (first-term poly)) 0)
             ((eq 0 (second-term poly)) 0)
             ((eq 1 (first-term poly))  (second-term poly))
             ((eq 1 (second-term poly)) (first-term  poly))
             (t (list '*
                 (simplifyn (first-term poly))
                 (simplifyn (second-term poly))))))
          ((eq '+ (function poly))
           (cond 
             ((constant-term? poly) 
              (+ (first-term poly) (second-term poly)))
             ((eq 0 (first-term poly))  (second-term poly))
             ((eq 0 (second-term poly)) (first-term poly))
             (t (list '+
                 (simplifyn (first-term poly))
                 (simplifyn (second-term poly))))))
          ((eq '- (function poly))
            (cond
              ((constant-term? poly) 
               (- (first-term poly) (second-term poly)))
              ((eq (first-term poly) (second-term poly)) 0)
              ((eq 0 (second-term poly)) (first-term poly))
              (t (list '-
                  (simplifyn (first-term  poly))
                  (simplifyn (second-term poly))))))
          ((eq '& (function poly))
            (cond
              ((constant-term? poly) 
               (& (first-term poly) (second-term poly)))
              ((eq (first-term poly) (second-term poly))
               (first-term poly))
              ((eq 0 (first-term  poly)) 0)
              ((eq 0 (second-term poly)) 0)
              ((eq *all-bits-set* (first-term  poly)) (second-term poly))
              ((eq *all-bits-set* (second-term poly)) (first-term  poly))
              (t (list '&
                  (simplifyn (first-term poly))
                  (simplifyn (second-term poly))))))
          ((eq '| (function poly))
            (cond
              ((constant-term? poly) 
               (| (first-term poly) (second-term poly)))
              ((eq 0 (first-term  poly)) (second-term poly))
              ((eq 0 (second-term poly)) (first-term  poly))
              ((eq *all-bits-set* (first-term  poly)) *all-bits-set*)
              ((eq *all-bits-set* (second-term poly)) *all-bits-set*)
              ((eq (first-term poly) (second-term poly)) (first-term poly))
              (t (list '|
                  (simplifyn (first-term poly))
                  (simplifyn (second-term poly))))))                   
          ((eq '^ (function poly))
            (cond
              ((constant-term? poly) 
               (^ (first-term poly) (second-term poly)))
              ((eq (first-term poly) (second-term poly)) 0)
              ((eq 0 (first-term  poly)) (second-term poly))
              ((eq 0 (second-term poly)) (first-term  poly))
              (t (list '^
                  (simplifyn (first-term poly))
                  (simplifyn (second-term poly))))))
          ((eq 'pow (function poly))
            (cond
              ((constant-term? poly)
               (pow (first-term poly) (second-term poly)))
              ((eq 0 (second-term poly)) 1)
              ((eq 1 (second-term poly)) (first-term poly))
              ((eq 0 (first-term poly)) 0)
              ((eq 1 (first-term poly)) 1) 
              (t (list 'pow
                       (simplifyn (first-term poly))
                       (simplifyn (second-term poly))))))
         (t (list (function poly) ; simplify the terms to an unknown expression
                  (simplifyn (first-term poly))
                  (simplifyn (second-term poly))))))) 

(define simplify
  (lambda
    "repeatedly simplify a tree of arity-n functions until there is no change."
    (poly)
    (let (poly-new ())
      (progn 
	(while
	  (progn
	    (set! poly-new (simplifyn poly))
	    (if (equal poly-new poly)
	      nil
	      (set! poly poly-new))))
      poly))))

(define infix->prefix
  (lambda
    "turn an infix expression into a postfix expression: (2 + 2) => (+ 2 2), arity has to be 2." 
    (poly)
    (cond
      ((list? poly) 
       (list 
         (cadr poly)
         (infix->prefix (car poly))
         (infix->prefix (caddr poly))))
      (t poly))))

(define prefix->infix
  (lambda
    "turn a prefix expression to a postfix expression: (+ 2 2) => (2 + 2). Arity has to be 2."
    (poly)
    (cond
    ((list? poly)
     (list
        (prefix->infix (cadr  poly))
        (car poly)
        (prefix->infix (caddr poly))))
    (t poly))))

