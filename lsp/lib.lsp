(define cadr (lambda (x) (car (cdr x))))
(define caddr (lambda (x) (car (cdr (cdr x)))))
(define cadddr (lambda (x) (car (cdr (cdr (cdr x))))))

(define gcd 
 (lambda (x y) 
  (if 
   (= 0 y) 
   x 
   (gcd y 
    (mod x y)))))

(define sumsqr (lambda (x y) (+ (* x x) (* y y))))

static expr primop_fake(expr args, lisp l); /* dummy for certain special forms */
static expr primop_add(expr args, lisp l);
static expr primop_sub(expr args, lisp l);
static expr primop_prod(expr args, lisp l);
static expr primop_div(expr args, lisp l);
static expr primop_mod(expr args, lisp l);
static expr primop_cdr(expr args, lisp l);
static expr primop_car(expr args, lisp l);
static expr primop_cons(expr args, lisp l);
static expr primop_numeq(expr args, lisp l);
static expr primop_printexpr(expr args, lisp l);

