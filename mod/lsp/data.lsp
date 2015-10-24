;;; a list of "databases" and functions which act on them ;;;

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
    (let 
      (d (date))
      (month (cadr (assoc (cadr d)  *months*)))
      (wd    (cadr (assoc (caddr d) *week-days*)))
      (mday  (cadddr d))
      (hrms  (cddddr d))
      (hrmss (join ":" (symbol->string (car hrms)) 
                       (symbol->string (cadr hrms)) 
                       (symbol->string (caddr hrms))))
      (join " " (symbol->string (car d)) month wd (symbol->string mday) hrmss))))

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

(define ascii-integers
 ;(0 . "\000") 
'((0000 . "")
  (0001 . "\001") (0002 . "\002") (0003 . "\003") (0004 . "\004")
  (0005 . "\005") (0006 . "\006") (0007 . "\007") (0010 . "\010")
  (0011 . "\011") (0012 . "\012") (0013 . "\013") (0014 . "\014") 
  (0015 . "\015") (0016 . "\016") (0017 . "\017") (0020 . "\020") 
  (0021 . "\021") (0022 . "\022") (0023 . "\023") (0024 . "\024") 
  (0025 . "\025") (0026 . "\026") (0027 . "\027") (0030 . "\030") 
  (0031 . "\031") (0032 . "\032") (0033 . "\033") (0034 . "\034") 
  (0035 . "\035") (0036 . "\036") (0037 . "\037") (0040 . "\040") 
  (0041 . "\041") (0042 . "\042") (0043 . "\043") (0044 . "\044") 
  (0045 . "\045") (0046 . "\046") (0047 . "\047") (0050 . "\050") 
  (0051 . "\051") (0052 . "\052") (0053 . "\053") (0054 . "\054") 
  (0055 . "\055") (0056 . "\056") (0057 . "\057") (0060 . "\060") 
  (0061 . "\061") (0062 . "\062") (0063 . "\063") (0064 . "\064") 
  (0065 . "\065") (0066 . "\066") (0067 . "\067") (0070 . "\070") 
  (0071 . "\071") (0072 . "\072") (0073 . "\073") (0074 . "\074") 
  (0075 . "\075") (0076 . "\076") (0077 . "\077") (0100 . "\100") 
  (0101 . "\101") (0102 . "\102") (0103 . "\103") (0104 . "\104") 
  (0105 . "\105") (0106 . "\106") (0107 . "\107") (0110 . "\110") 
  (0111 . "\111") (0112 . "\112") (0113 . "\113") (0114 . "\114") 
  (0115 . "\115") (0116 . "\116") (0117 . "\117") (0120 . "\120") 
  (0121 . "\121") (0122 . "\122") (0123 . "\123") (0124 . "\124") 
  (0125 . "\125") (0126 . "\126") (0127 . "\127") (0130 . "\130") 
  (0131 . "\131") (0132 . "\132") (0133 . "\133") (0134 . "\134") 
  (0135 . "\135") (0136 . "\136") (0137 . "\137") (0140 . "\140") 
  (0141 . "\141") (0142 . "\142") (0143 . "\143") (0144 . "\144") 
  (0145 . "\145") (0146 . "\146") (0147 . "\147") (0150 . "\150") 
  (0151 . "\151") (0152 . "\152") (0153 . "\153") (0154 . "\154") 
  (0155 . "\155") (0156 . "\156") (0157 . "\157") (0160 . "\160") 
  (0161 . "\161") (0162 . "\162") (0163 . "\163") (0164 . "\164") 
  (0165 . "\165") (0166 . "\166") (0167 . "\167") (0170 . "\170") 
  (0171 . "\171") (0172 . "\172") (0173 . "\173") (0174 . "\174") 
  (0175 . "\175") (0176 . "\176") (0177 . "\177")))

(define integer->ascii
  (lambda (x)
    (if (> x 0177) nil (cdr (assoc x ascii-integers)))))

