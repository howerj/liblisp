;;; Simple X11 test application ;;;
;; There is a lot of things that can be improved, such as
;; drawing different shapes, selecting different size shapes,
;; saving those shapes to a file, non-blocking events (which
;; requires changes in the X11 module), text drawing functions
;; and more.

(define w (create-window))

(define rectangle-width 4)
(define rectangle-height 4)
(define mouse-x (lambda (elist) (caddr elist)))
(define mouse-y (lambda (elist) (cadddr elist)))
(define key (lambda (elist) (cadr elist)))

(define is-mouse?
  (lambda (elist) 
    (and (integer? (mouse-x elist)) 
	 (integer? (mouse-y elist)))))

(define is-key?
  (lambda (elist)
    (string? (cadr elist))))

(define is-redraw?
  (lambda (elist)
    (car elist)))

(define redraw-list ())

(define add-rectangle
  (lambda (x y)
    (set! redraw-list (cons (list x y) redraw-list))))

(define redraw-rectangle
  (lambda (point)
    (draw-rectangle w (car point) (cadr point) rectangle-width rectangle-height)))

(define redraw
  (lambda ()
    (map1 redraw-rectangle redraw-list)))

(define redraw-file "objects.log")
; reload object list

(define load-redraw-list
  (lambda (save-file)
    (let 
      (file (open *file-in* save-file))
      (if file 
	(progn 
	  (set! redraw-list (read file))
	  ; test if this succeeded
	  (format *output* "loaded file '%s'\n%S\n" save-file redraw-list)
	  (redraw)
	  (close file)
	  t)
	(progn
	  (format *output* "could not load file '%s'\n" save-file)
	  nil)))))

(define save-redraw-list
  (lambda (save-file)
    (let
      (file (open *file-out* save-file))
      (if file
	(progn
	  (format file "%S\n" redraw-list)
	  (format *output* "save to file '%s'\n%S\n" save-file redraw-list)
	  t)
	nil))))

(load-redraw-list redraw-file)

(progn
	(let (event ())
		(progn 
			(set! event (select-input w))
			;(format *output* "%S\n" event)
			(if (is-mouse? event) 
			  (progn 
			    ;(format *output* "mouse\n")
			    (draw-rectangle w (mouse-x event) (mouse-y event) rectangle-width rectangle-height) 
			    (add-rectangle (mouse-x event) (mouse-y event))
			    ;(format *output* "%S\n" redraw-list)
			    nil)
			  nil)
			(if (is-redraw? event)
			  (progn
			    ; (format *output* "redraw\n")
			    (redraw)
			    nil)
			  nil)

			; must be last event
			(if (is-key? event)
			  (cond 
			    ((= (key event) "q") (return return))
			    ((= (key event) "s") (save-redraw-list redraw-file))
			    ; clear screen
			    ((= (key event) "c")
			     (progn
			       (set! redraw-list ())
			       (clear-window w)
			       nil))
			    (t (format *output* "key %s\n" (key event))))
			  nil)))
	loop
	t)

(save-redraw-list redraw-file)
