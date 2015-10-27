;;; SQL functions ;;;

(define sql-show-all 
  (compile
    "show all tables in an SQL database"
    (db) 
    (sql db "SELECT * FROM sqlite_master")))

