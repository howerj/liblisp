;;; Translation of:
;;; http://www.ibiblio.org/pub/academic/computer-science/history/pdp-11/rsx/decus/rsx85a/300340/prolog.lsp
;;; Another example
;;; http://wiki.call-cc.org/eggref/4/tiny-prolog

;;; @note I need to debug and understand the interpreter, the main goal of me
;;; adding this to the repository was to learn and understand how prolog works,
;;; so do not expect anything special with this interpreter - RJHowe
;;; @todo Debugging, finish porting, improve efficiency, add built primitives,
;;; adding to the database, etc
;;; @note This manual (http://www.maclisp.info/pitmanual/) will help in the
;;; porting
;;; @note In MacLisp (car nil) <=> nil

;; The following is tiny Prolog interpreter in MacLisp
;; written by Ken Kahn and modified for XLISP by David Betz.
;; It was inspired by other tiny Lisp-based Prologs of
;; Par Emanuelson and Martin Nilsson.
;; There are no side-effects anywhere in the implementation.
;; Though it is VERY slow of course.

(define
  prolog 
  (lambda "lispy prolog interpreter" 
    (database)
    (let (goal nil)
       (while 
         (/=
           (progn 
             (format *output* "Query? ") 
             (setq goal (read *input*))) 
             'error)
         (prove 
           (list 
             (rename-variables goal '(0)))
           '((bottom-of-environment))
           database
           1)))))

(define prove 
  (lambda "proves the conjunction of the list-of-goals in the current environment" 
    (list-of-goals environment database level)
      (cond (is-nil.list-of-goals ;; succeeded since there are no goals
             (progn 
               (print-bindings environment environment)
               (not (y-or-n-p "More (y/n)? "))))
            (t (try-each database database
                         cdr.list-of-goals car.list-of-goals
                         environment level)))))

(define try-each 
  (lambda "try each" 
    (database-left database goals-left goal environment level)
    (let
      (assertion nil)
      (new-environment nil)
      (cond 
        (is-nil.database-left nil) ;; fail since nothing left in database
        (t 
          (progn
            (setq assertion       (rename-variables car.database-left list.level))
            (setq new-environment (unify goal car.assertion environment))
            (cond (is-nil.new-environment ;; failed to unify
                       (try-each cdr.database-left database
                                 goals-left goal
                                 environment level))
                      ((prove (append (cdr assertion) goals-left)
                              new-environment
                              database
                              (+ 1 level)) nil)
                      (t (try-each (cdr database-left) database
                                   goals-left goal
                                   environment level)))))))))

(define unify 
  (lambda "unify variables" (x y environment)
    (let (new-environment nil)
      (progn
       (setq x (value x environment))
       (setq y (value y environment))
       (cond (variable-p.x (cons (list x y) environment))
             (variable-p.y (cons (list y x) environment))
             ((or is-atom.x is-atom.y)
                  (cond ((equal x y) environment)
                            (t nil)))
             (t 
               (progn (setq new-environment (unify car.x car.y environment))
                (cond 
                  (new-environment (unify cdr.x cdr.y new-environment))
                  (t nil)))))))))

(define value 
  (lambda "value" (x environment)
    (let (binding nil)
       (cond (variable-p.x
              (progn 
                (setq binding (assoc x environment))
                (cond (is-nil.binding x)
                    (t (value cadr.binding environment)))))
             (t x)))))

(define variable-p 
  (lambda "is variable p" (x)
    (cond
      (is-nil.x nil)
      (is-list.x (eq car.x '?))
      (t nil))))

(define rename-variables 
  (lambda "rename variables" 
    (term list-of-level)
    (progn
       ;(format *output* "TERM = %S, LEVEL = %S\n" term list-of-level)
       (cond ((variable-p term) (append term list-of-level))
             ((is-atom term) term)
             (t (cons (rename-variables car.term list-of-level)
                      (rename-variables cdr.term list-of-level)))))))

(define print-bindings 
  (lambda "print bindings" 
    (environment-left environment)
       (cond ((cdr environment-left)
              (progn 
                (cond 
                  ((= 0 (nth 2 caar.environment-left))
                   (format *output* 
                           "%S = %S\n"
                           cadr.caar.environment-left
                           (value caar.environment-left environment))))
                (print-bindings cdr.environment-left environment))))))

;; a sample database:
(define db 
  '(
    ((father madelyn ernest))
    ((mother madelyn virginia))
    ((father david arnold))
    ((mother david pauline))
    ((father rachel david))
    ((mother rachel madelyn))
    ((grandparent (? grandparent) (? grandchild))
     (parent 
       (? grandparent) (? parent))
     (parent (? parent) (? grandchild)))
    ((parent (? parent) (? child))
     (mother (? parent) (? child)))
    ((parent (? parent) (? child))
     (father (? parent) (? child)))))

;; the following are utilities
(define y-or-n-p 
  (lambda 
    "" 
    (prompt)
       (format *output* "%s" prompt)
       (eq (read *input*) 'y)))

;; start things going
;(prolog db)
