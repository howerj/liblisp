;;; Simple X11 test application ;;;
;; There is a lot of things that can be improved, such as
;; drawing different shapes, selecting different size shapes,
;; saving those shapes to a file, non-blocking events (which
;; requires changes in the X11 module), text drawing functions
;; and more.
;;
;; @todo Make something more useful

(define w (create-window))

(define rectangle-width 4)
(define rectangle-height 4)
(define mouse-x (lambda (elist) (caddr elist)))
(define mouse-y (lambda (elist) (cadddr elist)))
(define key (lambda (elist) (cadr elist)))

(define is-mouse
  (lambda (elist) 
    (and (is-integer (mouse-x elist)) 
	 (is-integer (mouse-y elist)))))

(define is-key
  (lambda (elist)
    (string? (cadr elist))))

(define is-redraw
  (lambda (elist)
    (car elist)))

(define redraw-list ())

(define add-rectangle
  (lambda (x y)
    (setq redraw-list (cons (list x y) redraw-list))))

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
	  (setq redraw-list (read file))
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

(let (event ())
  (while
	(progn 
		(setq event (select-input w))
		(if (is-mouse event) 
		  (progn 
		    (draw-rectangle w (mouse-x event) (mouse-y event) rectangle-width rectangle-height) 
		    (add-rectangle (mouse-x event) (mouse-y event)))
		  nil)
		(cond ((is-redraw event) (redraw)))

		; must be last event
		(if (is-key event)
		  (cond 
		    ((= (key event) "q") nil)
		    ((= (key event) "s") (save-redraw-list redraw-file))
		    ; clear screen
		    ((= (key event) "c")
		     (progn
		       (setq redraw-list ())
		       (clear-window w)
		       t))
		    (t (format *output* "key %s\n" (key event))))
		  t))))

(save-redraw-list redraw-file)
