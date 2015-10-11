;; Simple Eliza program -- see Norvig, _Paradigms of AI Programming_.
;; TODO: eliminate comments, map IDs, write->scribe, etc.
;; TODO: random-element argh!
;;       need to thread state through eliza loop
;;       need a random-update function - use an LFSR, I guess


(define (rule-pattern rule) (car rule))
(define (rule-answers rule) (cdr rule))

(define (get-rules)
  '(((hello)
     (How do you do -- please state your problem))
    ((I want)
     (What would it mean if you got -R-)
     (Why do you want -R-)
     (Suppose you got -R- soon))
    ((if)
     (Do you really think its likely that -R-)
     (Do you wish that -R-)
     (What do you think about -R-)
     (Really-- if -R-))
    ((I was)
     (Were you really?)
     (Perhaps I already knew you were -R-)
     (Why do you tell me you were -R- now?))
    ((I am)
     (In what way are you -R-)
     (Do you want to be -R-))
    ((because)
     (Is that the real reason?)
     (What other reasons might there be?)
     (Does that reason seem to explain anything else?))
    ((I feel)
     (Do you often feel -R-))
    ((I felt)
     (What other feelings do you have?))
    ((yes)
     (You seem quite positive)
     (You are sure)
     (I understand))
    ((no)
     (Why not?)
     (You are being a bit negative)
     (Are you saying no just to be negative?))
    ((someone)
     (Can you be more specific?))
    ((everyone)
     (Surely not everyone)
     (Can you think of anyone in particular?)
     (Who for example?)
     (You are thinking of a special person))
    ((perhaps)
     (You do not seem quite certain))
    ((are)
     (Did you think they might not be -R-)
     (Possibly they are -R-))
    (()
     (Very interesting)
     (I am not sure I understand you fully)
     (What does that suggest to you?)
     (Please continue)
     (Go on)
     (Do you feel strongly about discussing such things?))))

(define (eliza)
  (say '(Hello-- please state your problem))
  (eliza-loop))

(define (eliza-loop)
  (write '>)
  (eliza-loop-1 (read)))

(define (eliza-loop-1 sentence)
  (cond ((eq? the-eof-object sentence))
        (#t (say (eliza-answer sentence))
            (eliza-loop))))

(define (eliza-answer sentence)
  (check-rules (get-rules) sentence))

(define (check-rules rules sentence)
  (cond ((null? rules) (error 'cant-happen 'check-rules))
        (#t (try-rule (car rules) (cdr rules) sentence))))

(define (try-rule rule rules sentence)
  (try-rule-match (match (rule-pattern rule) sentence) rule rules sentence))

(define (try-rule-match env rule rules sentence)
  (cond (env (flatten (sublis (switch-roles env)
                              (random-element (rule-answers rule)))))
        (#t (check-rules rules sentence))))

(define (switch-roles words)
    ;; What's with all the quotes?
  (sublis '(('I . 'you) ('you . 'I) ('me . 'you) ('am . 'are))
          words))


;; Matching

;; If SUBLIST is a sublist of LST, then return an a-list with -R- bound to 
;; the part of LST after SUBLIST.  Else return #f.
(define (match sublist lst)
  (cond ((matches-head? sublist lst)
         (list1 (cons '-R- (after-head sublist lst))))
        ((null? lst) #f)
        (#t (match sublist (cdr lst)))))

(define (matches-head? alleged-head lst)
  (cond ((null? lst) (null? alleged-head))
        ((null? alleged-head) #t)
        ((eq? (car alleged-head) (car lst))
         (matches-head? (cdr alleged-head) (cdr lst)))
        (#t #f)))

(define (after-head head lst)
  (cond ((null? head) lst)
        (#t (after-head (cdr head) (cdr lst)))))


;; Help functions

(define (sublis a-list exp)
  (cond ((null? exp) '())
        ((pair? exp)
         (cons (sublis a-list (car exp))
               (sublis a-list (cdr exp))))
        (#t (sublis-1 (assq exp a-list) exp))))

(define (sublis-1 binding exp)
  (cond (binding (cdr binding))
        (#t exp)))
    
(define (assq key pairs)
  (cond ((null? pairs) #f)
        ((eq? (caar pairs) key) (car pairs))
        (#t (assq key (cdr pairs)))))

(define (flatten lst)
  (cond ((null? lst) '())
        ((null? (car lst)) (flatten (cdr lst)))
        ((atom? (car lst))
         (cons (car lst) (flatten (cdr lst))))
        (#t (append (flatten (car lst))
                    (flatten (cdr lst))))))

(define (say sentence)
  (write-each sentence)
  (newline))

(define (write-each xs)
  (cond ((null? xs))
        (#t (write (car xs))
            (write-char #\space)
            (write-each (cdr xs)))))

;; FIXME
(define random-element
  (lambda (lst)
    (list-ref lst (random (length lst)))))
