(define sql-show-all 
  (lambda (db) 
    (sql db "SELECT * FROM sqlite_master")))

