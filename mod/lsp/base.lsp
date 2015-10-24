;;; base software library ;;;

(define not        (lambda (x) (null? x)))
(define list?      (lambda (x) (type? *cons* x)))
(define atom?      (lambda (x) (if (list? x) nil t)))
(define float?     (lambda (x) (type? *float* x)))
(define integer?   (lambda (x) (type? *integer* x)))
(define io?        (lambda (x) (type? *io* x)))
(define hash?      (lambda (x) (type? *hash* x)))
(define string?    (lambda (x) (type? *string* x)))
(define procedure? (lambda (x) (type? *procedure* x)))
(define primitive? (lambda (x) (type? *primitive* x)))
(define char?      (lambda (x) (and (string? x) (= (length x) 1))))
(define dotted?    (lambda (x) (and (list? x) (not (list? (cdr x))))))
(define arithmetic?(lambda (x) (or (integer? x) (float? x))))

(define nil? (lambda (x) (if x nil t)))
(define null? ; weaker version of nil
  (lambda (x)
    (cond
      ((string? x)  (eq x ""))
      ((float? x)   (eq x 0.0))
      ((integer? x) (eq x 0))
      ((hash? x)    (eq (coerce *cons* x) nil))
      (t (eq nil x)))))  

; these are only defined because they are used elsewhere
(define caar   (lambda (x) (car (car x))))
(define cadr   (lambda (x) (car (cdr x))))
(define cdar   (lambda (x) (cdr (car x))))
(define cddr   (lambda (x) (cdr (cdr x))))
(define cadar  (lambda (x) (car (cdr (car x)))))
(define caddr  (lambda (x) (car (cdr (cdr x)))))
(define cdddr  (lambda (x) (cdr (cdr (cdr x)))))
(define cddddr (lambda (x) (cdr (cdr (cdr (cdr x))))))
(define caddar (lambda (x) (car (cdr (cdr (car x))))))
(define cadddr (lambda (x) (car (cdr (cdr (cdr x))))))

; substitute all "y" with "x" in list "z"
(define subst 
  (lambda (x y z)
    (if (atom? z)
      (if (eq z y) x z)
      (cons (subst x y (car z))
            (subst x y (cdr z))))))

; characters do not exist in this interpreter but we
; can treat strings of length one as characters
(define char? 
  (lambda (x)
    (and (string? x)
         (= 1 (length x)))))
; ¬(x ∧ y)
(define nand
  (lambda (x y)
    (or (not x) (not y))))

; ¬(x ∨ y)
(define nor
  (lambda (x y)
    (and (not x) (not y))))

; is x zero?
(define zero? 
  (lambda (x)
    (if (= x 0) t nil)))

; maximum of two arithmetic types or string
(define max
  (lambda (x y)
    (if (> x y) x y)))

; minimum of two arithmetic types or string
(define min
  (lambda (x y)
    (if (< x y) x y)))

; absolute value of a number
(define abs
  (lambda (x)
    (if (> 0 x) (* x -1) x)))

; the signum function
(define signum
  (lambda (x)
    (cond
      ((> x 0)  1)
      ((< x 0) -1)
      (t 0))))

; is a number positive?
(define positive?
  (lambda (x)
    (if (< x 0) nil t)))

; is a number negative?
(define negative?
  (lambda (x) (not (positive? x))))

; is a integer even?
(define even?
  (lambda (x)
    (if (= (% x 2) 0) t nil)))

; is a integer odd?
(define odd?
  (lambda (x)
    (not (even? x))))

; get the nth element of a list or a string
(define nth
  (lambda (i x)
    (if (zero? i)
      (if (string? x) (scar x) (car x))
      (nth (- i 1) (if (string? x) (scdr x) (cdr x))))))

; append two lists
(define append
  (lambda (x y)
    (cond
      ((null? x)  y)
      (t (cons (car x) 
               (append (cdr x) y))))))

; turn two lists into list of pairs (a-list)
(define pair 
  (lambda (x y) 
    (cond 
      ((and (null? x) (null? y)) nil)
      ((and (not (atom? x)) (not (atom? y)))
       (cons (list (car x) (car y))
             (pair (cdr x) (cdr y)))))))

; get last element in list
(define last 
  (lambda (x)
    (if (list? (cdr x))
      (last (cdr x))
      (car x))))

; is pair? eg. '(a b)
(define pair? 
  (lambda (x)
    (cond 
      ((atom? x) nil)
      ((null? x) nil)
      ((null? (cdr x)) nil)
      ((null? (cdr (cdr x))) t)
      (t nil))))

(define <= (lambda (x y) (or (< x y) (= x y))))
(define >= (lambda (x y) (or (> x y) (= x y))))

; sort a list, super inefficiently
(define sort 
  (let
   (sort-insert (lambda (x l)
    (if 
     (null? l)
      (list x)
       (if 
        (<= x (car l))
         (cons x l)
         (cons 
          (car l)
          (sort-insert x (cdr l)))))))
  (lambda (l)
    (if (null? l)
      nil
      (sort-insert 
        (car l) 
        (sort (cdr l)))))))

; inexact floating point equality
(define float-equal
  (lambda (x y)
    (<
      (abs (- x y))
      0.00001)))

; rewind an IO port to point to the beginning of its input or output
(define rewind (lambda (port) (seek port 0 *seek-set*)))

; flatten a tree
(define flatten 
  (lambda (l)
    (cond
      ((null? l) nil)
      ((atom? l) (list l))
      (t (append (flatten (car l))
                 (flatten (cdr l)))))))

; turn a string into a list of characters
(define explode
  (lambda (s)
    (coerce *cons* s)))

; turn a list of strings into a string
(define implode
  (lambda (s)
    (if
      (eq nil (cdr s))
      (scons (car s) "")
      (scons (car s) (implode (cdr s))))))

; get a single line from an input port
(define get-line
  (lambda (in)
    (get-delim in "\n")))

; list of atoms?
(define lat?
  (lambda (l)
    (cond
      ((null? l) t)
      ((atom? (car l)) (lat? (cdr l)))
      (t nil))))

; find 'a in list
(define member?
  (lambda (a lat)
    (cond
      ((null? lat) ())
      (t (or (equal (car lat) a)
                (member? a (cdr lat)))))))

; remove member from list
(define remove-member 
  (lambda (a lat)
    (cond
      ((null? lat) nil)
      ((equal (car lat) a) (remove-member a (cdr lat)))
      (t (cons (car lat)
                  (remove-member a (cdr lat)))))))

; returns 't if str matches the pattern
(define regex 
  (lambda (pattern str)
    (car (regex-span pattern str))))

; print a newline
(define newline (lambda () (put *output* "\n")))

; exclude all the element from 0 to k in a list
(define list-tail
  (lambda (l k)
    (cond
      ((zero? k) l)
      (t (list-tail (cdr l) (- k 1))))))

; get all the elements from 0 to k in a list
(define list-head 
  (lambda (l k)
    (cond
      ((zero? k) (cons (car l) nil))
      (t (cons (car l) (list-head (cdr l) (- k 1)))))))

; make a sub sequence of a list
(define sublist
  (lambda (l start end)
    (list-tail (list-head l end) start)))

; pick a random element from a list
(define random-element
  (lambda (x)
    (if (list? x)
      (let
        (ll (% (abs (random)) (length x)))
        (car (sublist x ll ll)))
      x)))

; Walk two isomorphic trees and apply a function to each pair
; of elements in each tree. It would be nice to make this function
; more generic than it already is.
(define tree-walk
  (lambda (f x y)
        (cond 
          ((and (atom? x) (atom? y))
           (f x y))
          ((and (list? x) (list? y))
           (and 
             (tree-walk f (car x) (car y))
             (tree-walk f (cdr x) (cdr y))))
          (t nil))))

; are two trees equal?
(define equal
  (lambda (x y)
    (tree-walk eq x y)))

; are two trees structurally isomorphic
(define struct-iso?
  (lambda (x y)
    (tree-walk (lambda (x y) t) x y)))

; are two trees structurally isomorphic and each elements of
; the same type?
(define type-iso?
  (lambda (x y)
    (tree-walk (lambda (x y) (eq (type-of x) (type-of y))) x y)))

(define inc (lambda (x) (+ x 1))) ; increment a value
(define dec (lambda (x) (- x 1))) ; decrement a value

(define bye  (lambda () (exit)))  ; quit the interpreter
(define quit (lambda () (exit)))  ; quit the interpreter

(define symbol->string (lambda (s) (coerce *string* s)))

(define gensym-counter 0) ; *GLOBAL* counter for gensym
(define gensym ; generate a new *unique* symbol
  (lambda ()
      (set! gensym-counter (inc gensym-counter))
      (coerce *symbol*
        (join
          "-"
          "GENSYM"
          (symbol->string gensym-counter)))))

(define map1
  (lambda (f l)
    (if (nil? l)
      nil
      (cons (f (car l))
            (map1 f (cdr l))))))

(define square
  (lambda (x)
    (* x x)))

; greatest common divisor
(define gcd
 (lambda (x y) 
  (if 
   (= 0 y) 
   x 
   (gcd y 
    (% x y)))))

; lowest common multiple
(define lcm
  (lambda (x y)
    (* (/ x (gcd x y)) y)))

; the factorial function, naive version
(define factorial
  (lambda (x)
    (if (< x 2)
        1
        (* x (factorial (- x 1))))))

(define arithmetic-mean
  (lambda (l)
    (/ (foldl + l) (length l))))

(define average
  (lambda (l)
    (arithmetic-mean l)))

(define geometric-mean ; requires math module
  (lambda (l)
    (pow (foldl * l) (/ 1. (length l)))))

(define median
  (lambda (l)
    (let 
      (len (length l))
      (half (/ len 2))
      (if 
        (odd? len)
        (nth half (sort l))
        (let 
          (middle (sublist (sort l) (- half 1) half))
          (/ (+ (car middle) (cadr middle)) 2))))))

(define variance
  (lambda (l)
    (/ (foldl + (map1 square l)) (length l))))

(define standard-deviation ; requires math module
  (lambda (l)
    (sqrt (variance l))))

(define degrees->radians
  (lambda (deg)
    (* (/ pi 180.) deg)))

(define radians->degrees
  (lambda (rad)
    (* (/ 180. pi) rad)))

(define cartesian->polar ; (x . y) => (magnitude . radians)
  (lambda (cart)
    (cons 
      (sqrt (+ (square (car cart)) (square (cdr cart))))
      (atan (/ (coerce *float* (cdr cart)) (car cart))))))

(define polar->cartesian ; (magnitude . radians) => (x . y)
  (lambda (pol)
    (cons
      (* (cos (cdr pol)) (car pol))
      (* (sin (cdr pol)) (car pol)))))


; is a variable defined or not?
(define defined?
  (lambda (x)
   (if
    (assoc x (top-environment))
     t
     nil)))

(define lower->upper (lambda (s) (tr "" "abcdefghijklmnopqrstuvwxyz" "ABCDEFGHIJKLMNOPQRSTUVWXYZ" s)))
(define upper->lower (lambda (s) (tr "" "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" s)))

(define unix->windows (lambda (p) (tr "" "/" "\\\\" p))) 
(define windows->unix (lambda (p) (tr "" "\\\\" "/" p)))

(define help
  (lambda 
    (func) 
    (map1
      (lambda (x) (format *output* "%s\n" x)) 
      (split ":" (documentation-string func)))))

