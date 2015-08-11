#!./lisp

# A lot of these functions need to be rewritten so as to
# be tail recursive, or if looping constructs were added, with
# them. Also if letrec were added...

# These functions come from various sources, such as "The Roots Of Lisp"
# essay, the book "The little schemer" and fragments from all over.

# Todo
# * list equality
# * list structural equality
# * unify, prolog interpreter

# doesn't work correctly yet
(define assert
  (flambda (test)
    (let* (te (begin (print (env)) (eval (car test) (env))))
      (if te
        nil
        (begin (put "Assertion failed:\n\t")
               (print (car test))
               (put "\n")
               (throw -1))))))

# These functions should be a part of the interpreter.
(define cadr (lambda (x) (car (cdr x))))
(define caar (lambda (x) (car (car x))))
(define cadar (lambda (x) (car (cdr (car x)))))
(define caddr (lambda (x) (car (cdr (cdr x)))))
(define caddar (lambda (x) (car (cdr (cdr (car x))))))

(define type?      (lambda (type-enum x) (eq type-enum (type-of x))))
(define list?      (lambda (x) (type? *cons* x)))
(define atom?      (lambda (x) (if (list? x) nil t)))
(define float?     (lambda (x) (type? *float* x)))
(define integer?   (lambda (x) (type? *integer* x)))
(define symbol?    (lambda (x) (type? *symbol* x)))
(define string?    (lambda (x) (type? *string* x)))
(define io?        (lambda (x) (type? *io* x)))
(define hash?      (lambda (x) (type? *hash* x)))
(define string?    (lambda (x) (type? *string* x)))
(define procedure? (lambda (x) (type? *procedure* x)))
(define primitive? (lambda (x) (type? *primitive* x)))
(define char?      (lambda (x) (and (string? x) (= (length x) 1))))
(define dotted?    (lambda (x) (and (list? x) (not (list? (cdr x))))))

(define gcd 
 (lambda (x y) 
  (if 
   (= 0 y) 
   x 
   (gcd y 
    (% x y)))))

(define lcm
  (lambda (x y)
    (* (/ x (gcd x y)) y)))

(define factorial
  (lambda (x)
    (if (< x 2)
        1
        (* x (factorial (- x 1))))))

(define subst 
  (lambda (x y z)
    (if (atom? z)
      (if (eq z y) x z)
      (cons (subst x y (car z))
            (subst x y (cdr z))))))

(define null? 
  (lambda (x) 
    (eq x nil)))

(define not # null? and not are the same in this lisp
  (lambda (x)
    (null? x)))

# @bug Incorrect, evaluates all args
(define and
  (lambda (x y)
    (if x
      (if y t nil)
       nil)))

# @bug Incorrect, evaluates all args
(define or
  (lambda (x y)
    (cond (x t)
          (y t)
          (t nil))))

(define char? 
  (lambda (x)
    (and (string? x)
         (= 1 (length x)))))

(define nand
  (lambda (x y)
    (or (not x) (not y))))

(define nor
  (lambda (x y)
    (and (not x) (not y))))

(define zero? 
  (lambda (x)
    (if (= x 0) t nil)))

(define max
  (lambda (x y)
    (if (> x y) x y)))

(define min
  (lambda (x y)
    (if (< x y) x y)))

(define abs
  (lambda (x)
    (if (> 0 x) (* x -1) x)))

(define signum
  (lambda (x)
    (cond
      ((> x 0)  1)
      ((< x 0) -1)
      (t 0))))

(define positive?
  (lambda (x)
    (if (< x 0) nil t)))

(define negative?
  (lambda (x) (not (positive? x))))

(define even?
  (lambda (x)
    (if (= (% x 2) 0) t nil)))

(define odd?
  (lambda (x)
    (not (even? x))))

(define nth
  (lambda (i x)
    (if (zero? i)
      (car x)
      (nth (- i 1) (cdr x)))))

(define append
  (lambda (x y)
    (cond
      ((null? x)  y)
      (t (cons (car x) 
               (append (cdr x) y))))))

(define pair # turn two lists into list of pairs (a-list)
  (lambda (x y) 
    (cond 
      ((and (null? x) (null? y)) nil)
      ((and (not (atom? x)) (not (atom? y)))
       (cons (list (car x) (car y))
             (pair (cdr x) (cdr y)))))))

(define last # get last element in list
  (lambda (x)
    (if (list? (cdr x))
      (last (cdr x))
      (car x))))

(define pair? # is pair? eg. '(a b)
  (lambda (x)
    (cond 
      ((atom? x) nil)
      ((null? x) nil)
      ((null? (cdr x)) nil)
      ((null? (cdr (cdr x))) t)
      (t nil))))

(define sort 
  (letrec
   (*sort-insert (lambda (x l)
    (if 
     (null? l)
      (list x)
       (if 
        (<= x (car l))
         (cons x l)
         (cons 
          (car l)
          (*sort-insert x (cdr l)))))))
  (lambda (l)
    (if (null? l)
      nil
      (*sort-insert 
        (car l) 
        (sort (cdr l)))))))

(define float-equal
  (lambda (x y)
    (<
      (fabs (- x y))
      0.00001)))

##############################################################################

(define exit (lambda () (error -1)))

(define flatten # flatten a tree
  (lambda (l)
    (cond
      ((null? l) nil)
      ((atom? l) (list l))
      (t (append (flatten (car l))
                 (flatten (cdr l)))))))

(define print-me (flambda (x) x))

(define explode
  (lambda (s)
    (coerce *cons* s)))

(define implode # turn a list of chars into a string
  (lambda (s)
    (if
      (eq nil (cdr s))
      (scons (car s) "")
      (scons (car s) (implode (cdr s))))))

(define get-line
  (lambda (in)
    (get-delim in "\n")))

(define lat? # list of atoms?
  (lambda (l)
    (cond
      ((null? l) t)
      ((atom? (car l)) (lat? (cdr l)))
      (t nil))))

(define member? # find 'a in lat
  (lambda (a lat)
    (cond
      ((null? lat) ())
      (t (or (eq (car lat) a)
                (member? a (cdr lat)))))))

(define remove-member # remove member from lat
  (lambda (a lat)
    (cond
      ((null? lat) nil)
      ((eq (car lat) a) (cdr lat))
      (t (cons (car lat)
                  (remove-member a (cdr lat)))))))

(define <= (lambda (x y) (or (< x y) (= x y))))
(define >= (lambda (x y) (or (> x y) (= x y))))

# Example of a "flambda" expression
(define newline
  (flambda (x)
        (cond ((= (length x) 1) 
               (if (output? (eval (car x) (environment)))
                   (put (eval (car x) (environment)) "\n")
                   (begin
                     (put "newline expects (IO)\n")
                     (error 1))))
              ((= (length x) 0)
               (put "\n"))
              (t (begin 
                   (put "newline expects () or (IO))")
                   (error 1))))))

# Evaluate an entire file
# (eval-file STRING)
# (eval-file INPUT-PORT)
(define eval-file
  (lambda (file)

    # This bit does the evaluation of a file input object
    (letrec 
      (eval-file-inner
       (lambda (S)
        (if (eof? S) 
          nil
          (let* 
            (x (read S))
            (if (eq x 'error) 
              nil
              (begin 
                (print (eval x))
                (newline)
                (eval-file-inner S)))))))

    (cond 
      # If we have been given a potential file name, try to open it
      ((or (string? file) (symbol? file))
        (let* (file-handle (open *file-in* file))
          (if (input? file-handle)
            (begin (eval-file-inner file-handle)
                   (close file-handle) # Close file!
                   nil)
            (begin (put "could not open file for reading\n")
                   'error))))
      # We must have been passed an input file port
      ((input? file)
       (eval-file-inner file)) # Do not close file!
      # User error
      (t 
        (begin 
          (put "Not a file name or a output IO type\n") 
          'error))))))
        

##############################################################################
# Sets
# To do
#  * Power set

(define set? # Is list a set, eg. no repeated symbols
  (lambda (lat)
    (cond
      ((null? lat) t)
      ((member? (car lat) (cdr lat)) nil)
      (t (set? (cdr lat))))))

(define make-set # Make a set from a list (remove repeats)
  (letrec
   (*make-set # Does the work of making a set from a list
     (lambda (lat)
       (cond
         ((null? lat) nil)
         ((member? (car lat) (cdr lat)) (*make-set (cdr lat)))
         (t (cons (car lat) (*make-set (cdr lat)))))))
   (lambda (lat)
    (*make-set (sort lat)))))

(define subset? # A ⊆ B
  (lambda (A B)
    (cond 
      ((null? A) t)
      ((member? (car A) B)
       (subset? (cdr A) B))
      (t nil))))

(define eqset? # A = B
  (lambda (A B)
    (and (subset? A B)
         (subset? B A))))

(define intersects? # (A ∩ B)?
  (lambda (A B)
    (cond
      ((null? A) nil)
      ((member? (car A) B) t)
      (t (intersects? (cdr A) B)))))

(define intersection # A ∩ B 
  (lambda (A B)
    (cond
      ((null? A) nil)
      ((member? (car A) B)
       (cons 
         (car A) 
         (intersection (cdr A) B)))
      (t (intersection (cdr A) B)))))

(define union # A ∪ B
  (lambda (A B)
    (cond
      ((null? A) B)
      ((member? (car A) B)
       (union (cdr A) B))
      (t (cons 
           (car A) 
           (union (cdr A) B))))))

(define A\B # A \ B
  (lambda (A B)
    (cond
      ((null? A) nil)
      ((member? (car A) B)
       (A\B (cdr A) B))
      (t (cons 
           (car A) 
           (A\B (cdr A) B))))))

(define relative-difference # B \ A
  (lambda (A B)
    (A\B B A)))

(define symmetric-difference # A △ B
  (lambda (A B)
    (union (A\B A B) (A\B B A))))

(define list-tail
  (lambda (l k)
    (cond
      ((zero? k) l)
      (t (list-tail (cdr l) (- k 1))))))

(define list-head 
  (lambda (l k)
    (cond
      ((zero? k) (cons (car l) nil))
      (t (cons (car l) (list-head (cdr l) (- k 1)))))))

(define sublist
  (lambda (l start end)
    (list-tail (list-head l end) start)))

# Test whether a variable is defined or not
(define defined?
  (flambda (x)
    (let* (ret (assoc (car x) (environment))) (if ret (cdr ret) nil))))

(define log-msg
  (flambda (x)
     (let* (out (eval (car x)))
     (if (output? out) 
       (begin 
         (print out (append (date) (cdr x)))
         (put out "\n")
         (append '(logged) (cdr x)))
       'error))))

##############################################################################

