;; eliza.lsp

(define wpred
  (lambda (w)
    (member w '(WHY WHAT WHO WHERE WHEN))))

(define dpred
  (lambda (w)
    (member w '(DO CAN SHOULD WOULD COULD))))

(define verb
  (lambda (w)
    (member w '(GO HAVE BE TRY TAKE EAT HELP MAKE GET JUMP
		    WRITE TYPE FILL PUT TURN COMPUTE THINK
		    DRINK BLINK CRASH CRUNCH ADD))))

; not finished
(define ematch
  (lambda (p s)
    (cond
      ((nil? p) (nil? s))
      ((atom? (car p))
       (and s
	    (equal (car p) (car s))))
      (t nil))))

(define me-to-you
  (lambda 
    (w)
    (cond
      ((= w 'I)     'YOU)
      ((= w 'ME)    'YOU)
      ((= w 'YOU)   'ME)
      ((= w 'YOUR)  'MY)
      ((= w 'YOURS) 'MINE)
      ((= w 'MINE)  'YOURS)
      ((= w 'AM)    'ARE)
      (t w))))

(define to-me-to-you ; no slacking
  (lambda
    (l)
    (map1 me-to-you l)))

(define generic-punts
  '("PLEASE GO ON"
    "TELL ME MORE"
    "INTERESTING...GO ON"
    "HOW DOES THAT MAKE YOU FELL"
    "DOES THAT CONCERN YOU?"
    "WHAT DO YOU BELIEVE THAT INDICATES?"
    "ARE YOU CERTAIN ABOUT THAT?"))

(define eliza-inner 
  (lambda (line)
    (cond
      ((member 'DREAM line) (format *output* "I'D LIKE VERY MUCH TO KNOW MORE ABOUT YOUR DREAMS.\n"))
      ((member 'LOVE  line) (format *output* "TELL ME MORE ABOUT YOUR LOVE LIFE.\n"))
      ((member 'HATE  line) (format *output* "HATE IS A POWERFUL EMOTION.\n"))
      ((member 'SEX   line) (format *output* "GO ON...\n"))
      ((member 'MAYBE line) (format *output* "BE MORE DECISIVE.\n"))
      ((member 'NO    line) (format *output* "DON'T BE SO NEGATIVE!\n"))
      ((member 'WOMEN line) (format *output* "TELL ME MORE ABOUT THE WOMEN IN YOUR LIFE.\n"))
      ((member 'MEN   line) (format *output* "TELL ME MORE ABOUT THE MEN IN YOUR LIFE.\n"))
;     ((member 'YOU   line) (progn (map1 (lambda (x) (format *output* "%S " x)) line) (newline)))
      (t (format *output* "%s\n" (random-element generic-punts))))))

(define eliza 
  (lambda ()
    (let 
      (line ())
      (word ())
      (progn
	(format *output* "Hello, %s, how are you today?\n" "INSERT-NAME")
	(while (not (= error (set! word (read *input*))))
	       (if (= word '.)
		(eliza-inner (to-me-to-you (reverse line)))
		(set! line (cons (coerce *symbol* (lower->upper word)) line))))))
    'GOODBYE))


