/* sql module, not completed*/
#include <assert.h>
#include <liblisp.h>
#include <sqlite3.h>
#include <stdlib.h>

extern lisp *lglobal; /* from main.c */

static int ud_sql;
static void construct(void) __attribute__((constructor));
static void destruct(void) __attribute__((destructor));

static void ud_sql_free(cell *f) {
        if(!lisp_is_cell_closed(f))
                sqlite3_close(userval(f));  
        free(f);
}

static int ud_sql_print(io *o, unsigned depth, cell *f) {
        return printerf(NULL, o, depth, 
                        "%B<SQL-STATE:%d:%s>%t", 
                                userval(f), lisp_is_cell_closed(f) ? "CLOSED" : "OPEN");
}

static int sql_callback(void *obin, int argc, char **argv, char **azColName)
{       
        int i;
        cell *ob;
        assert(obin);
        ob = *((cell**)obin);
        for (i = 0; i < argc; i++)
                ob = cons(lglobal, 
                                cons(lglobal, 
                                        mkstr(lglobal, lstrdup(azColName[i])),
                                        argv[i] ? mkstr(lglobal, lstrdup(argv[i])) : gsym_nil()), ob);
        *((cell **)obin) = ob;
        return 0;
}

static cell *subr_sqlopen(lisp *l, cell *args) {
        sqlite3 *db;
        assert(l == lglobal);
        if(!cklen(args, 1) || !is_asciiz(car(args)))
               RECOVER(l, "\"expected (string)\" '%S", args); 
        if(sqlite3_open(strval(car(args)), &db)) {
                printerf(l, lisp_get_logging(l), 0, "(sql-error \"%s\")\n", sqlite3_errmsg(db));
                sqlite3_close(db);
                return gsym_error();
        }
        return mkuser(l, db, ud_sql);
}

static cell *subr_sql(lisp *l, cell *args) {
        char *errmsg;
        cell *head = gsym_nil();
        assert(l == lglobal);
        if(!cklen(args, 2) || !is_usertype(car(args), ud_sql) || !is_asciiz(CADR(args)) || lisp_is_cell_closed(car(args)))
                RECOVER(l, "\"expected (sql-database string)\" '%S", args); 
        if(sqlite3_exec(userval(car(args)), strval(CADR(args)), sql_callback, &head, &errmsg) != SQLITE_OK) {
                printerf(l, lisp_get_logging(l), 0, "(sql-error \"%s\")\n", errmsg);
                sqlite3_free(errmsg);
                return gsym_error();
        }
        return head;
}

static cell *subr_sqlclose(lisp *l, cell *args) {
        assert(l == lglobal);
        if(!cklen(args, 1) || !is_usertype(car(args), ud_sql) || lisp_is_cell_closed(car(args)))
                RECOVER(l, "\"expected (sql-database)\" '%S", args);
        sqlite3_close(userval(car(args)));
        return gsym_tee();
}

static void construct(void) {
        ud_sql = newuserdef(lglobal, ud_sql_free, NULL, NULL, ud_sql_print);
        lisp_add_subr(lglobal, "sql",       subr_sql);
        lisp_add_subr(lglobal, "sql-open",  subr_sqlopen);
        lisp_add_subr(lglobal, "sql-close", subr_sqlclose);
}

static void destruct(void) {
}
