;;; base software library ;;;

(define null? ; weaker version of nil
  (lambda 
    "is x an empty object? (\"\", 0, 0.0 or empty hash, ...)"
    (x)
    (cond
      ((string? x)  (eq x ""))
      ((float? x)   (eq x 0.0))
      ((integer? x) (eq x 0))
      ((hash? x)    (eq (coerce *cons* x) nil))
      (t (eq nil x)))))  

(define not        (lambda (x) (null? x)))
(define list?      (lambda "is x a list?" (x) (type? *cons* x)))
(define atom?      (lambda "is x an atom? (not a list)" (x) (if (list? x) nil t)))
(define float?     (lambda "is x a floating point number?" (x) (type? *float* x)))
(define integer?   (lambda "is x a integer type?" (x) (type? *integer* x)))
(define io?        (lambda "is x a input/output port?" (x) (type? *io* x)))
(define hash?      (lambda "is x a hash table?" (x) (type? *hash* x)))
(define string?    (lambda "is x a string?" (x) (type? *string* x)))
(define procedure? (lambda "is x a procedure?" (x) (type? *procedure* x)))
(define primitive? (lambda "is x a primitive type?" (x) (type? *primitive* x)))
(define char?      (lambda "is x a character? (string of length 1)" (x) (and (string? x) (= (length x) 1))))
(define dotted?    (lambda "is x a dotted pair?" (x) (and (list? x) (not (list? (cdr x))))))
(define arithmetic?(lambda "is x an arithmetic type? (integer or float)" (x) (or (integer? x) (float? x))))

(define nil? (lambda "is x nil?" (x) (if x nil t)))

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

(define subst 
  (lambda 
    "substitute all y for x in tree z" 
    (x y z)
    (if (atom? z)
      (if (eq z y) x z)
      (cons (subst x y (car z))
            (subst x y (cdr z))))))

; ¬(x ∧ y)
(define nand
  (lambda (x y)
    (or (not x) (not y))))

; ¬(x ∨ y)
(define nor
  (lambda 
    "compute \"nor\" of 'x and 'y" 
    (x y)
    (and (not x) (not y))))

(define zero? 
  (lambda (x)
    "is x zero?"
    (if (= x 0) t nil)))

(define max
  (lambda 
    "maximum of two arithmetic types or string"
    (x y)
    (if (> x y) x y)))

(define min
  (lambda 
    "minimum of two arithmetic types or string"
    (x y)
    (if (< x y) x y)))

(define abs
  (lambda 
    "absolute value of a number"
    (x)
    (if (> 0 x) (* x -1) x)))

(define signum
  (lambda 
    "the signum function"
    (x)
    (cond
      ((> x 0)  1)
      ((< x 0) -1)
      (t 0))))

(define positive?
  (lambda 
    "is x a positive number?"
    (x)
    (if (< x 0) nil t)))

(define negative?
  (lambda "is x a negative number?" (x) (not (positive? x))))

(define even?
  (lambda "is x an even integer?" (x)
    (if (= (% x 2) 0) t nil)))

(define odd?
  (lambda "is x an odd integer?" (x)
    (not (even? x))))

(define nth
  (lambda 
    "get the nth (i) element of a string or a list (x)" 
    (i x)
    (if (zero? i)
      (if (string? x) (scar x) (car x))
      (nth (- i 1) (if (string? x) (scdr x) (cdr x))))))

(define append
  (lambda 
    "append two lists together"
    (x y)
    (cond
      ((null? x)  y)
      (t (cons (car x) 
               (append (cdr x) y))))))

(define pair 
  (lambda 
    "turn two lists into list of pairs (a-list): (pair '(a b c) '(1 2 3)) => ((a 1) (b 2) (c 3))"
    (x y) 
    (cond 
      ((and (null? x) (null? y)) nil)
      ((and (not (atom? x)) (not (atom? y)))
       (cons (list (car x) (car y))
             (pair (cdr x) (cdr y)))))))

(define last 
  (lambda 
    "get the last element in a list"
    (x)
    (if (list? (cdr x))
      (last (cdr x))
      (car x))))

(define pair? 
  (lambda 
    "is x a pair? eg. (a b)"
    (x)
    (cond 
      ((atom? x) nil)
      ((nil? x) nil)
      ((nil? (cdr x)) nil)
      ((nil? (cdr (cdr x))) t)
      (t nil))))

(define <= (lambda "is x less than or equal to y" (x y) (or (< x y) (= x y))))
(define >= (lambda "is x greater than or equal to y" (x y) (or (> x y) (= x y))))

(define sort
  (let
   (sort-insert (lambda (x l)
    (if 
     (nil? l)
      (list x)
       (if 
        (<= x (car l))
         (cons x l)
         (cons 
          (car l)
          (sort-insert x (cdr l)))))))
  (lambda 
    "A super inefficient sort on a list of integers/floats or strings"
    (l)
    (if (nil? l)
      nil
      (sort-insert 
        (car l) 
        (sort (cdr l)))))))

; @warning this is not actually correct, see <http://floating-point-gui.de/errors/comparison/>
(define float-equal
  (lambda 
    "inexact floating point equality"
    (x y)
    (<
      (abs (- x y))
      0.00001)))

(define rewind 
  (lambda
    "rewind an IO port to point to the beginning of its input or output"
    (port) 
    (seek port 0 *seek-set*)))

(define flatten 
  (lambda 
    "flatten a tree (flatten '((a b (c) d) e)) => (a b c d e)"
    (l)
    (cond
      ((null? l) nil)
      ((atom? l) (list l))
      (t (append (flatten (car l))
                 (flatten (cdr l)))))))

(define explode
  (lambda 
    "turn a string into a list of characters"
    (s)
    (coerce *cons* s)))

(define implode
  (lambda
    "turn a list of characters into a string"
    (s)
    (if
      (eq nil (cdr s))
      (scons (car s) "")
      (scons (car s) (implode (cdr s))))))

(define get-line
  (lambda 
    "read in a single line from an input port"
    (in)
    (get-delim in "\n")))

(define slurp
  (lambda 
    "read in an entire file (from an input port) into a string"
    (in)
    (get-delim in *eof*)))

(define lat?
  (lambda 
    "is 'l a list of atoms?"
    (l)
    (cond
      ((null? l) t)
      ((atom? (car l)) (lat? (cdr l)))
      (t nil))))

(define member?
  (lambda 
    "find an atom in a list of atoms"
    (a lat)
    (cond
      ((null? lat) ())
      (t (or (equal (car lat) a)
                (member? a (cdr lat)))))))

(define remove-member 
  (lambda 
    "remove a member from a list of atoms"
    (a lat)
    (cond
      ((null? lat) nil)
      ((equal (car lat) a) (remove-member a (cdr lat)))
      (t (cons (car lat)
                  (remove-member a (cdr lat)))))))

(define regex 
  (lambda 
    "match a pattern on a string, return 't if found, 'nil if not."
    (pattern str)
    (car (regex-span pattern str))))

(define newline (lambda "print a newline" () (put *output* "\n")))

(define list-tail
  (lambda 
    "exclude all the elements from 0 to k in a list"
    (l k)
    (cond
      ((zero? k) l)
      (t (list-tail (cdr l) (- k 1))))))

(define list-head 
  (lambda 
    "get all the elements from 0 to k in a list"
    (l k)
    (cond
      ((zero? k) (cons (car l) nil))
      (t (cons (car l) (list-head (cdr l) (- k 1)))))))

(define sublist
  (lambda 
    "get a sub sequence from a list"
    (l start end)
    (list-tail (list-head l end) start)))

(define random-element
  (lambda 
    "pick a random element from a list"
    (x)
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

(define equal
  (lambda 
    "are two trees equal?"
    (x y)
    (tree-walk eq x y)))

(define struct-iso?
  (lambda 
    "are two trees structurally isomorphic?"
    (x y)
    (tree-walk (lambda (x y) t) x y)))

(define type-iso?
  (lambda 
    "are two trees both structurally and type isomorphic?"
    (x y)
    (tree-walk (lambda (x y) (eq (type-of x) (type-of y))) x y)))

(define inc (lambda "increment a value" (x) (+ x 1)))
(define dec (lambda "decrement a value" (x) (- x 1)))

(define bye  (lambda "quit the interpreter" () (exit)))  ; quit the interpreter
(define quit (lambda "quit the interpreter" () (exit)))  ; quit the interpreter

(define symbol->string (lambda "coerce a symbol into a string" (s) (coerce *string* s)))

(define gensym-counter 0) ; *GLOBAL* counter for gensym
(define gensym
  (lambda "generate a unique, new symbol" ()
      (set! gensym-counter (inc gensym-counter))
      (coerce *symbol*
        (join
          "-"
          "GENSYM"
          (symbol->string gensym-counter)))))

(define map1
  (lambda 
    "map a function onto a list returning a list of the function applied to each element"
    (f l)
    (if (nil? l)
      nil
      (cons (f (car l))
            (map1 f (cdr l))))))

(define square
  (lambda 
    "square a number"
    (x)
    (* x x)))

(define frandom 
  (lambda "create a random number in the range of 0.0 and 1.0" () 
    (fabs (/ (coerce *float* (random)) *random-max*))))

(define sum-of-squares 
  (lambda 
    "return the sum of the squares of two numbers"
    (x y) 
    (+ (square x) (square y))))

(define gcd
 (lambda "compute the greatest common divisor of two integers" (x y) 
  (if 
   (= 0 y) 
   x 
   (gcd y 
    (% x y)))))

(define lcm
  (lambda "compute the lowest common multiple of two integers" (x y)
    (* (/ x (gcd x y)) y)))

(define factorial
  (lambda "compute the factorial of a integer" (x)
    (if (< x 2)
        1
        (* x (factorial (- x 1))))))

(define arithmetic-mean
  (lambda "return the arithmetic mean of a list of numbers" (l)
    (/ (foldl + l) (length l))))

(define average
  (lambda "return the arithmetic mean of a list of numbers" (l)
    (arithmetic-mean l)))

(define geometric-mean 
  (lambda "compute the geometric mean of a list of numbers (requires math module)" (l)
    (pow (foldl * l) (/ 1. (length l)))))

(define median
  (lambda "compute the median value of a list of numbers" (l)
    (let 
      (len (length l))
      (half (/ len 2))
      (if 
        (odd? len)
        (nth half (sort l))
        (let 
          (middle (sublist (sort l) (dec half) half))
          (/ (+ (car middle) (cadr middle)) 2))))))

(define variance
  (lambda "compute the variance of a list of numbers" (l)
    (/ (foldl + (map1 square l)) (length l))))

(define standard-deviation
  (lambda "compute the standard deviation of a list of numbers (requires math module)" (l)
    (sqrt (variance l))))

(define degrees->radians
  (lambda "convert degrees into radians" (deg)
    (* (/ pi 180.) deg)))

(define radians->degrees
  (lambda "convert radians into degrees" (rad)
    (* (/ 180. pi) rad)))

(define cartesian->polar 
  (lambda 
    "convert Cartesian coordinates (x . y) into polar coordinates (magnitude . angle), angle is in radians"
    (cart)
    (cons 
      (sqrt (+ (square (car cart)) (square (cdr cart))))
      (atan (/ (coerce *float* (cdr cart)) (car cart))))))

(define polar->cartesian 
  (lambda 
    "convert polar coordinates (magnitude . angle), angle is in radians, into Cartesian coordinates (x. y)"
    (pol)
    (cons
      (* (cos (cdr pol)) (car pol))
      (* (sin (cdr pol)) (car pol)))))

(define defined?
  (lambda 
    "is a variable defined or not? (argument should be quoted)"
    (x)
    (if
      (assoc x (top-environment))
      t
      nil)))

(define lower->upper 
  (lambda 
    "convert a string to upper case" 
    (s) 
    (tr "" "abcdefghijklmnopqrstuvwxyz" "ABCDEFGHIJKLMNOPQRSTUVWXYZ" s)))

(define 
  upper->lower 
  (lambda 
    "convert a string to lower case"
    (s) 
    (tr "" "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" s)))

(define unix->windows 
  (lambda 
    "convert a Unix file path to a Windows file path"
    (p) 
    (tr "" "/" "\\\\" p))) 

(define windows->unix 
  (lambda 
    "convert a Windows file path to a Unix file path"
    (p) 
    (tr "" "\\\\" "/" p)))

(define help
  (lambda 
    "print out as much information about a function as is known"
    (func) 
    (map1
      (lambda (x) (format *output* "%s\n" x)) 
      (split ":" (documentation-string func)))))

