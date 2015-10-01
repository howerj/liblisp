;;; base software library ;;;
; A lot of these functions need to be rewritten so as to
; be tail recursive, or if looping constructs were added, with
; them. 

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
  (letrec
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

; the simplest flambda expression
(define print-me (flambda (x) x))

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

; apply a function to all the car elements of a list, returning
; a list made up of all of those evaluated arguments
(define mapcar 
  (flambda (args)
        (letrec
          (func (eval (car args)))
          (body (cadr args))
          (mapper (lambda (l)
              (if (cdr l)
                (cons
                  (func (car l))
                  (mapper (cdr l)))
                (cons (func (car l)) nil))))
          (mapper (eval body)))))
 
; example of a "flambda" expression
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

; Test whether a variable is defined or not
(define defined?
  (flambda (x)
    (let* (ret (assoc (car x) (environment))) (if ret (cdr ret) nil))))

; log a message to an output port
(define log-msg
  (flambda (x)
     (let* (out (eval (car x)))
     (if (output? out) 
       (format out "%S\n" (append (date) (cdr x)))
       (begin (put "expected output port\n") 'error)))))

; pick a random element from a list
(define random-element
  (lambda (x)
    (if (list? x)
      (let*
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

(define make-string (lambda (s) (coerce *string* s)))

(define gensym-counter 0) ; *GLOBAL* counter for gensym
(define gensym ; generate a new *unique* symbol
  (lambda ()
    (begin
      (set! gensym-counter (inc gensym-counter))
      (coerce *symbol*
        (join
          "-"
          "GENSYM"
          (make-string gensym-counter)
;         (make-string (abs (random)))
              )))))

(define *months* ; months of the year association list
  '((0 January)  (1 February)  (2 March) 
    (3 April)    (4 May)       (5 June) 
    (6 July)     (7 August)    (8 September)
    (9 October)  (10 November) (11 December)))

(define *week-days* ; days of the week association list
  '((0 Sunday)  (1 Monday)  (2 Tuesday) (3 Wednesday)
    (4 Thurday) (5 Friday)  (6 Saturday)))

(define date-string ; make a nicely formatted date string
  (lambda ()
    (let* 
      (d (date))
      (month (cadr (assoc (cadr d)  *months*)))
      (wd    (cadr (assoc (caddr d) *week-days*)))
      (mday  (cadddr d))
      (hrms  (cddddr d))
      (hrmss (join ":" (make-string (car hrms)) 
                       (make-string (cadr hrms)) 
                       (make-string (caddr hrms))))
      (join " " (make-string (car d)) month wd (make-string mday) hrmss))))

; association list of type numbers and a description of that type
(define *type-names* 
 (pair
   (list 
      *integer*     *symbol*    *cons*        
      *string*      *hash*      *io*          
      *float*       *procedure* *primitive*   
      *f-procedure*)
   (list 
      "Integer"               "Symbol"               "Cons list" 
      "String"                "Hash"                 "Input/Output port"    
      "Floating point number" "Lambda procedure"     "Primitive subroutine" 
      "F-Expression")))

(define type-name ; Get a string representing the name of a type
  (lambda (x) 
    (cadr 
      (assoc (type-of x) *type-names*))))

