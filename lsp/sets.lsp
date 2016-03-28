;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; Sets
; @todo Power set
; @todo Functions should return sorted sets

(define is-set 
  (compile
    "is a list a set (no repeated symbols)"
    (lat)
    (cond
      (is-nil.lat t)
      ((member car.lat cdr.lat) nil)
      (t (is-set cdr.lat)))))

(define make-set 
  (let
   (_make-set ; Does the work of making a set from a list
     (lambda (lat)
       (cond
         ((is-nil lat) nil)
         ((member car.lat cdr.lat) (_make-set cdr.lat))
         (t (cons car.lat (_make-set cdr.lat))))))
   (lambda "make a set from a list of strings *or* numbers" (lat)
    (_make-set sort.lat))))

(define subset ; A ⊆ B
  (compile
    "is set A a subset of set B?"
    (A B)
    (cond 
      (is-nil.A t)
      ((member car.A B)
       (subset cdr.A B))
      (t nil))))

(define eq-set ; A = B
  (lambda 
    "is set A equal to set B?" 
    (A B)
    (and (subset A B)
         (subset B A))))

(define intersects ; (A ∩ B)?
  (lambda (A B)
    "does set A intersect with set B?"
    (cond
      (is-nil.A nil)
      ((member car.A B) t)
      (t (intersects cdr.A B)))))

(define intersection ; A ∩ B 
  (lambda (A B)
    "compute the intersection of sets A and B"
    (cond
      ((is-nil A) nil)
      ((member car.A B)
       (cons 
         car.A 
         (intersection cdr.A B)))
      (t (intersection cdr.A B)))))

(define union ; A ∪ B
  (lambda 
    "compute the union of sets A and B"
    (A B)
    (cond
      ((is-nil A) B)
      ((member car.A B)
       (union  cdr.A B))
      (t (cons 
           car.A 
           (union cdr.A B))))))

(define A\B ; A \ B
  (lambda (A B)
    (cond
      (is-nil.A nil)
      ((member car.A B)
       (A\B cdr.A B))
      (t (cons 
           car.A 
           (A\B cdr.A B))))))

(define relative-difference ; B \ A
  (compile "compute the relative difference of two sets" (A B)
    (A\B B A)))

; A △ B
(define symmetric-difference
  (compile
    "compute the symmetric difference of two sets"
    (A B)
    (union (A\B A B) (A\B B A))))

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

