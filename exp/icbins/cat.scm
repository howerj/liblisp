(define (main)
  (cat (read_char)))

(define (cat c)
  (cond ((is_eof_object c))
        (#t (write_char c)
            (cat (read_char)))))
