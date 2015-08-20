;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Sets
; To do
;  * Power set

; Is list a set, eg. no repeated symbols
(define set? 
  (lambda (lat)
    (cond
      ((null? lat) t)
      ((member? (car lat) (cdr lat)) nil)
      (t (set? (cdr lat))))))

; Make a set from a list (remove repeated elements)
(define make-set 
  (letrec
   (*make-set ; Does the work of making a set from a list
     (lambda (lat)
       (cond
         ((null? lat) nil)
         ((member? (car lat) (cdr lat)) (*make-set (cdr lat)))
         (t (cons (car lat) (*make-set (cdr lat)))))))
   (lambda (lat)
    (*make-set (sort lat)))))

; A ⊆ B
(define subset? 
  (lambda (A B)
    (cond 
      ((null? A) t)
      ((member? (car A) B)
       (subset? (cdr A) B))
      (t nil))))

; A = B
(define eqset?
  (lambda (A B)
    (and (subset? A B)
         (subset? B A))))

; (A ∩ B)?
(define intersects?
  (lambda (A B)
    (cond
      ((null? A) nil)
      ((member? (car A) B) t)
      (t (intersects? (cdr A) B)))))

; A ∩ B 
(define intersection
  (lambda (A B)
    (cond
      ((null? A) nil)
      ((member? (car A) B)
       (cons 
         (car A) 
         (intersection (cdr A) B)))
      (t (intersection (cdr A) B)))))

; A ∪ B
(define union
  (lambda (A B)
    (cond
      ((null? A) B)
      ((member? (car A) B)
       (union (cdr A) B))
      (t (cons 
           (car A) 
           (union (cdr A) B))))))
; A \ B
(define A\B
  (lambda (A B)
    (cond
      ((null? A) nil)
      ((member? (car A) B)
       (A\B (cdr A) B))
      (t (cons 
           (car A) 
           (A\B (cdr A) B))))))

; B \ A
(define relative-difference
  (lambda (A B)
    (A\B B A)))

; A △ B
(define symmetric-difference
  (lambda (A B)
    (union (A\B A B) (A\B B A))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

