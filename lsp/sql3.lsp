;;; SQL functions ;;;

(define sql-show-all 
  (lambda 
    "show all tables in an SQL database"
    (db) 
    (sql db "SELECT * FROM sqlite_master")))

