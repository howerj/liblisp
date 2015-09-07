; Differentiation of polynomials and symbolic simplification
; From http://shrager.org/llisp/, which is the tutorial for
; an old lisp called "P-LISP" which ran on the Apple II.

; The symbolic differentiation function uses these rules:
; D[0,x]=0
; D[x,x]=1
; D[(u+v),x]=D[u,x]+D[v,x]
; D[(u-v),x]=D[u,x]-D[v,x]
; D[(uv),x]=uD[v,x]+vD[u,x]
; D[(u^n),x]=nu^(n-1)D[u,x]
 
(define function   (lambda (poly) (car poly)))
(define first-term  (lambda (poly) (cadr poly)))
(define second-term (lambda (poly) (caddr poly)))

(define derive
  (lambda (poly var)
    (cond
      ((atom? poly) (dervatom poly var))
      ((eq '+ (function poly))
       (dervsum poly var))
      ((eq '- (function poly))
       (dervminus poly var))
      ((eq '* (function poly))
       (dervprod poly var))
      ((eq 'pow (function poly))
       (dervpow poly var)))))

(define dervatom
  (lambda (poly var)
    (cond
      ((eq poly var) 1)
      (t 0))))

(define dervsum
  (lambda (poly var)
    (list '+
          (derive (first-term poly) var)
          (derive (second-term poly) var))))

(define dervminus
  (lambda (poly var)
    (list '-
          (derive (first-term poly) var)
          (derive (second-term poly) var))))

(define dervprod
  (lambda (poly var)
    (list '+
          (list '*
                (first-term poly)
                (derive (second-term poly) var))
          (list '*
                (second-term poly)
                (derive (first-term poly) var)))))

(define dervpow
  (lambda (poly var)
    (list '*
          (list '*
                (second-term poly)
                (list 'pow
                      (first-term poly)
                      (- (second-term poly) 1))
                (derive (first-term poly) var)))))

; The simplification function uses these rules:
;
;    Lisp form      Simplified form
;    ---------      ---------------
;    *   ? 0              0
;    *   0 ?              0
;    *   1 ?              ?
;    *   ? 1              ?
;    +   0 ?              ?
;    +   ? 0              ?
;    -   ? 0              ?
;    pow ? 0              1
;    pow ? 1              ?
;    pow 0 ?              0
;    pow 1 ?              1
 
(define simplify1 (lambda (poly)
  (cond
       ((null? poly) nil)
       ((atom? poly) poly)
       ((eq '* (function poly))
         (cond
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
         (cond ((eq 0 (first-term poly))
                (simplify1 (second-term poly)))
               ((eq 0 (second-term poly))
                (simplify1 (first-term poly)))
               (t (list '+
                    (simplify1 (first-term poly))
                    (simplify1 (second-term poly))))))
       ((eq '- (function poly))
        (cond
         ((eq 0 (second-term poly))
          (simplify1 (first-term poly)))
         (t (list '-
                (simplify1 (first-term  poly))
                (simplify1 (second-term poly))))))
       ((eq 'pow (function poly))
        (cond
          ((eq 0 (second-term poly)) 1)
           ((eq 1 (second-term poly))
            (simplify1 (first-term poly)))
          ((eq 0 (first-term poly)) 0)
          ((eq 1 (first-term poly)) 1) ))
       (t poly))))

; Repeatedly apply simplify1 until there is no change
(define simplify
  (lambda (poly)
    (let* (poly1 (simplify1 poly))
      (cond 
        ((nil? poly1) poly)
        ((equal poly poly1)
         poly)
        (t (simplify poly1))))))

