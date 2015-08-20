; Differentiation of polynomials
; from http://shrager.org/llisp/

; D[0,x]=0
; D[x,x]=1
; D[(u+v),x]=D[u,x]+D[v,x]
; D[(u-v),x]=D[u,x]-D[v,x]
; D[(uv),x]=uD[v,x]+vD[u,x]
; D[(u^n),x]=nu^(n-1)D[u,x]
 
(define function   (lambda (poly) (car poly)))
(define first-term  (lambda (poly) (cadr poly)))
(define second-term (lambda (poly) (caddr poly)))

(define derv 
  (lambda (poly var)
    (cond
      ((atom? poly) (dervatom poly var))
      ((eq '+ (function poly))
       (dervsum poly var))
      ((eq '- (function poly))
       (dervminus poly var))
      ((eq '* (function poly))
       (dervprod poly var))
      ((eq 'exp (function poly))
       (dervexp poly var)))))

(define dervatom
  (lambda (poly var)
    (cond
      ((eq poly var) 1)
      (t 0))))

(define dervsum
  (lambda (poly var)
    (list '+
          (derv (first-term poly) var)
          (derv (second-term poly) var))))

(define dervminus
  (lambda (poly var)
    (list '-
          (derv (first-term poly) var)
          (derv (second-term poly) var))))

(define dervprod
  (lambda (poly var)
    (list '+
          (list '*
                (first-term poly)
                (derv (second-term poly) var))
          (list '*
                (second-term poly)
                (derv (first-term poly) var)))))

(define dervexp
  (lambda (poly var)
    (list '*
          (list '*
                (second-term poly)
                (list 'exp
                      (first-term poly)
                      (- (second-term poly) 1))
                (derv (first-term poly) var)))))


; Lisp form      Simplified form
; ---------      ---------------
; *   ? 0              0
; *   0 ?              0
; *   1 ?              ?
; *   ? 1              ?
; +   0 ?              ?
; +   ? 0              ?
; -   ? 0              ?
; exp ? 0              1
; exp ? 1              ?
; exp 0 ?              0
; exp 1 ?              1
 
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
       ((eq 'exp (function poly))
        (cond
          ((eq 0 (second-term poly)) 1)
           ((eq 1 (second-term poly))
            (simplify1 (first-term poly)))
          ((eq 0 (first-term poly)) 0)
          ((eq 1 (first-term poly)) 1) ))
       (t poly))))


