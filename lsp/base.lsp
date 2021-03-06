;;; base software library ;;;
(define not          
  (compile 
    "if x is-nil" 
    (x) 
    (if x nil t)))

(define is-list      
  (compile 
    "is x a is-list" 
    (x) 
    (is-type *cons* x)))

(define is-atom      
  (compile 
    "is x an is-atom (not a list)" 
    (x) 
    (if (is-list x) nil t)))

(define is-float     
  (compile 
    "is x a floating point number?" 
    (x) 
    (is-type *float* x)))

(define is-integer   
  (compile 
    "is x a integer is-type" 
    (x) 
    (is-type *integer* x)))

(define is-io        
  (compile 
    "is x a input/output port?" 
    (x) 
    (is-type *io* x)))

(define is-hash      
  (compile 
    "is x a hash table?" 
    (x) 
    (is-type *hash* x)))

(define is-string    
  (compile 
    "is x a is-string" 
    (x) 
    (is-type *string* x)))

(define is-procedure 
  (compile 
    "is x a is-procedure" 
    (x) 
    (is-type *procedure* x)))

(define is-primitive 
  (compile 
    "is x a primitive is-type" 
    (x) 
    (is-type *primitive* x)))

(define is-char      
  (compile 
    "is x a character? (string of length 1)" 
    (x) 
    (and is-string.x (= length.x 1))))

(define is-dotted    
  (compile 
    "is x a dotted pair?" 
    (x) 
    (and 
      is-list.x 
      not.is-list.cdr.x)))

(define is-arithmetic 
  (compile "is x an arithmetic is-type (integer or float)" (x) (or (is-integer x) (is-float x))))

(define is-empty
  (compile
    "is x an empty object? (\"\", 0, 0.0 or empty hash, ...)"
    (x)
    (cond
      (is-string.x  (eq x ""))
      (is-float.x   (eq x 0.0))
      (is-integer.x (eq x 0))
      (is-hash.x    (eq (coerce *cons* x) nil))
      (t not.x))))  

; @note this should be replaced by syntax in "read.c"
(define caar   (compile "caar"   (x) car.car.x))
(define cadr   (compile "cadr"   (x) car.cdr.x))
(define cdar   (compile "cdar"   (x) cdr.car.x))
(define cddr   (compile "cddr"   (x) cdr.cdr.x))
(define cadar  (compile "cadar"  (x) car.cdr.car.x))
(define caddr  (compile "caddr"  (x) car.cdr.cdr.x))
(define cdddr  (compile "cddr"   (x) cdr.cdr.cdr.x))
(define cddddr (compile "cddddr" (x) cdr.cdr.cdr.cdr.x))
(define caddar (compile "caddar" (x) car.cdr.cdr.car.x))
(define cadddr (compile "cadddr" (x) car.cdr.cdr.cdr.x))
(define cadadr (compile "cadadr" (x) car.cdr.car.cdr.x))

(define /= (compile "not equal" (x y) (not (= x y))))

(define subst 
  (compile
    "substitute all y for x in tree z" 
    (x y z)
    (if (is-atom z)
      (if (eq z y) x z)
      (cons (subst x y car.z)
	    (subst x y cdr.z)))))

(define nand ; ¬(x ∧ y)
  (compile "not-and of two arguments" 
	   (x y)
	   (or not.x not.y)))

(define nor ; ¬(x ∨ y)
  (compile
    "compute \"nor\" of 'x and 'y" 
    (x y)
    (and not.x not.y)))

(define is-zero 
  (compile 
    "is x is-zero"
    (x)
    (if (= x 0) t nil)))

(define max
  (compile
    "maximum of two arithmetic types or string"
    (x y)
    (if (> x y) x y)))

(define min
  (compile
    "minimum of two arithmetic types or string"
    (x y)
    (if (< x y) x y)))

(define abs
  (compile
    "absolute value of a number"
    (x)
    (if (> 0 x) (* x -1) x)))

(define signum
  (compile
    "the signum function"
    (x)
    (cond
      ((> x 0)  1)
      ((< x 0) -1)
      (t 0))))

(define is-positive
  (compile
    "is x a positive number?"
    (x)
    (if (< x 0) nil t)))

(define is-negative
  (compile "is x a negative number?" (x) (not (is-positive x))))

(define is-even
  (compile "is x an even is-integer" (x)
	   (if (= (% x 2) 0) t nil)))

(define is-odd
  (compile 
    "is x an odd is-integer" 
    (x)
    not.is-even.x))

(define nth
  (compile
    "get the nth (i) element of a string or a list (x)" 
    (i x)
    (if (is-zero i)
      (if (is-string x) scar.x car.x)
      (nth (- i 1) (if (is-string x) scdr.x cdr.x)))))

(define append
  (compile
    "append two lists together"
    (x y)
    (cond
      (is-nil.x  y)
      (t (cons 
	   car.x 
	   (append cdr.x y))))))

(define pair 
  (compile
    "turn two lists into list of pairs (a-list): (pair '(a b c) '(1 2 3)) => ((a 1) (b 2) (c 3))"
    (x y) 
    (cond 
      ((and (is-nil x) (is-nil y)) nil)
      ((and (not (is-atom x)) (not (is-atom y)))
       (cons (list (car x) (car y))
	     (pair (cdr x) (cdr y)))))))

(define last 
  (compile
    "get the last element in a list"
    (x)
    (if (is-list cdr.x)
      (last cdr.x)
      car.x)))

(define is-pair
  (compile
    "is x a pair? eg. (a b)"
    (x)
    (cond 
      (is-atom.x nil)
      (is-nil.x nil)
      (is-nil.cdr.x nil)
      (is-nil.cdr.cdr.x t)
      (t nil))))

(define <= 
  (compile 
    "is x less than or equal to y" 
    (x y) 
    (or 
      (< x y) 
      (= x y))))

(define >= 
  (compile 
    "is x greater than or equal to y" 
    (x y) 
    (or 
      (> x y) 
      (= x y))))

(let
  (sort-insert 
    (compile "" 
      (x l)
      (if 
	is-nil.l
	(list x)
	(if 
	  (<= x car.l)
	  (cons x l)
	  (cons 
	    (car l)
	    ('sort-insert x (cdr l)))))))
  (define sort
    (compile
      "A super inefficient sort on a list of integers/floats or strings"
      (l)
      (if is-nil.l
	nil
	(sort-insert 
	  car.l 
	  (sort cdr.l))))))

; @warning this is not actually correct, 
;          see <http://floating-point-gui.de/errors/comparison/>
(define float-equal
  (compile
    "inexact floating point equality"
    (x y)
    (<
      (abs (- x y))
      0.00001)))

(define rewind 
  (compile
    "rewind an IO port to point to the beginning of its input or output"
    (port) 
    (seek port 0 *seek-set*)))

(define flatten 
  (compile
    "flatten a tree (flatten '((a b (c) d) e)) => (a b c d e)"
    (l)
    (cond
      (is-nil.l nil)
      (is-atom.l list.l)
      (t (append (flatten car.l)
		 (flatten cdr.l))))))

(define explode
  (compile
    "turn a string into a list of characters"
    (s)
    (coerce *cons* s)))

(define implode
  (compile
    "turn a list of characters into a string"
    (s)
    (if
      is-nil.cdr.s
      (scons car.s "")
      (scons car.s (implode cdr.s)))))

(define get-line
  (compile
    "read in a single line from an input port"
    (in)
    (get-delim in "\n")))

(define slurp
  (compile
    "read in an entire file (from an input port) into a string"
    (in)
    (get-delim in *eof*)))

(define is-list-of-atoms
  (compile
    "is 'l a list of atoms?"
    (l)
    (cond
      (is-nil.l t)
      (is-atom.car.l (is-list-of-atoms (cdr l)))
      (t nil))))

(define member
  (compile
    "find an atom in a list of atoms"
    (a lat)
    (cond
      ((is-nil lat) ())
      (t (or (equal car.lat a)
	     (member a cdr.lat))))))

(define remove-member 
  (compile
    "remove a member from a list of atoms"
    (a lat)
    (cond
      (is-nil.lat nil)
      ((equal car.lat a) (remove-member a cdr.lat))
      (t (cons car.lat
	       (remove-member a cdr.lat))))))

(define newline (compile "print a newline" () (put *output* "\n")))

(define list-tail
  (compile
    "exclude all the elements from 0 to k in a list"
    (l k)
    (cond
      ((is-zero k) l)
      (t (list-tail (cdr l) (- k 1))))))

(define list-head 
  (compile
    "get all the elements from 0 to k in a list"
    (l k)
    (cond
      ((is-zero k) (cons (car l) nil))
      (t (cons (car l) (list-head (cdr l) (- k 1)))))))

(define sublist
  (compile
    "get a sub sequence from a list"
    (l start end)
    (list-tail (list-head l end) start)))

(define random-element
  (compile
    "pick a random element from a list"
    (x)
    (if is-list.x
      (let
	(ll (% (abs (random)) length.x))
	(car (sublist x ll ll)))
      x)))

; Walk two isomorphic trees and apply a function to each pair
; of elements in each tree. It would be nice to make this function
; more generic than it already is, with pre, in, post or breadth
; first search.
(define tree-walk
  (compile "walk a tree and apply a function returning a bool" (f x y)
	   (cond 
	     ((and (is-atom x) (is-atom y))
	      (f x y))
	     ((and (is-list x) (is-list y))
	      (and 
		(tree-walk f (car x) (car y))
		(tree-walk f (cdr x) (cdr y))))
	     (t nil))))

(define equal
  (compile
    "are two trees equal?"
    (x y)
    (tree-walk eq x y)))

(define partially-equal
  (compile
    "are two trees equal? The symbol '? can be used to match any atom in either tree"
    (x y)
    (tree-walk (lambda (x y) (or (= x y) (or (= x '?) (= y '?)))) x y)))

(define x-partially-equal
  (compile
    "are two trees equal? The symbol \"?\" when it appears in x can match any atom in y"
    (x y)
    (tree-walk (lambda (x y) (or (= x y) (= x '?))) x y)))

(define is-struct-iso
  (compile
    "are two trees structurally isomorphic?"
    (x y)
    (tree-walk (lambda (x y) t) x y)))

(define is-type-iso
  (compile
    "are two trees both structurally and type isomorphic?"
    (x y)
    (tree-walk (lambda (x y) (eq (type-of x) (type-of y))) x y)))

(define inc 
  (compile 
    "increment a value" 
    (x) 
    (+ x 1)))

(define dec 
  (compile "decrement a value" 
	   (x) 
	   (- x 1)))

(define bye  
  (compile 
    "quit the interpreter" 
    () 
    (exit)))

(define quit 
  (compile 
    "quit the interpreter" 
    () 
    (exit)))

(define symbol->string 
  (compile 
    "coerce a symbol into a string" 
    (s) 
    (coerce *string* s)))

(define gensym-counter 0) ; *GLOBAL* counter for gensym
(define gensym
  (lambda "generate a unique, new symbol" ()
    (setq gensym-counter (inc gensym-counter))
    (coerce *symbol*
	    (join
	      "-"
	      (list "GENSYM"
		    (symbol->string gensym-counter))))))

(define map1
  (compile
    "map a function onto a list returning a list of the function applied to each element"
    (f l)
    (if is-nil.l
      nil
      (cons (f car.l)
	    (map1 f cdr.l)))))

(define square
  (compile
    "square a number"
    (x)
    (* x x)))

(define frandom 
  (compile "create a random number in the range of 0.0 and 1.0" () 
	   (fabs (/ (coerce *float* (random)) *random-max*))))

(define sum-of-squares 
  (compile
    "return the sum of the squares of two numbers"
    (x y) 
    (+ (* x x) (* y y))))

(define gcd
  (compile "compute the greatest common divisor of two integers" (x y) 
	   (if 
	     (= 0 y) 
	     x 
	     (gcd y 
		  (% x y)))))

(define lcm
  (compile "compute the lowest common multiple of two integers" (x y)
	   (* (/ x (gcd x y)) y)))

(define factorial
  (compile "compute the factorial of a integer" (x)
	   (if (< x 2)
	     1
	     (* x (factorial (- x 1))))))

(define arithmetic-mean
  (compile "return the arithmetic mean of a list of numbers" (l)
	   (/ (foldl + l) (length l))))

(define average
  (compile "return the arithmetic mean of a list of numbers" (l)
	   (arithmetic-mean l)))

(define geometric-mean 
  (lambda "compute the geometric mean of a list of numbers (requires math module)" (l)
    (pow (foldl * l) (/ 1. (length l)))))

(define median
  (compile "compute the median value of a list of numbers" (l)
	   (let 
	     (len (length l))
	     (half (/ len 2))
	     (if 
	       (is-odd len)
	       (nth half (sort l))
	       (let 
		 (middle (sublist (sort l) (dec half) half))
		 (/ (+ (car middle) (cadr middle)) 2))))))

(define variance
  (compile "compute the variance of a list of numbers" (l)
	   (/ (foldl + (map1 square l)) (length l))))

(define standard-deviation
  (lambda "compute the standard deviation of a list of numbers (requires math module)" (l)
    (sqrt (variance l))))

(define degrees->radians
  (compile "convert degrees into radians" (deg)
	   (* (/ pi 180.) deg)))

(define radians->degrees
  (compile "convert radians into degrees" (rad)
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
    "convert polar coordinates (magnitude . angle), angle is in radians, into Cartesian coordinates (x . y)"
    (pol)
    (cons
      (* (cos (cdr pol)) (car pol))
      (* (sin (cdr pol)) (car pol)))))

(define is-defined
  (compile
    "is a variable defined or not? (argument should be quoted)"
    (x)
    (if
      (assoc x (top-environment))
      t
      nil)))

(define lower->upper 
  (compile
    "convert a string to upper case" 
    (s) 
    (tr "" "abcdefghijklmnopqrstuvwxyz" "ABCDEFGHIJKLMNOPQRSTUVWXYZ" s)))

(define 
  upper->lower 
  (compile
    "convert a string to lower case"
    (s) 
    (tr "" "ABCDEFGHIJKLMNOPQRSTUVWXYZ" "abcdefghijklmnopqrstuvwxyz" s)))

(define unix->windows 
  (compile
    "convert a Unix file path to a Windows file path"
    (p) 
    (tr "" "/" "\\\\" p))) 

(define windows->unix 
  (compile
    "convert a Windows file path to a Unix file path"
    (p) 
    (tr "" "\\\\" "/" p)))

(define quote-list
  (flambda "return all arguments unevaluated" (x) x))

(define defun
  (flambda "define a new function" (x)
	   (let 
	     (name (car x))
	     (doc  (cadr x))   ; documentation string
	     (args (caddr x))  ; function arguments
	     (code (cadddr x))
	     (eval (list define name (list lambda doc args code)) (environment)))))

(define identity 
  (lambda "return its argument" (x) x))

(define swap-bits-in-byte
  (compile
    "swap the bits in a byte"
    (b)
    (progn
      (setq b (| (>> (& b 0xF0) 4) (<< (& b 0x0F) 4)))
      (setq b (| (>> (& b 0xCC) 2) (<< (& b 0x33) 2)))
      (setq b (| (>> (& b 0xAA) 1) (<< (& b 0x55) 1)))
      b)))

(define swap-bytes-16
  (compile
    "swap bytes in a 16 bit value"
    (x)
    (progn
      (setq x (| (>> (& 0xFF00 x) 8) (<< (& 0x00FF 8) 8)))
      x)))

(define swap-bytes-32
  (compile
    "swap bytes in a 32 bit value"
    (x)
    (progn
      (setq x (| (>> (& 0xFFFF0000 x) 16) (<< (& 0x0000FFFF x) 16)))
      (setq x (| (>> (& 0xFF00FF00 x)  8) (<< (& 0x00FF00FF x)  8)))
      x)))


