/* sql module, not completed*/
#include <liblisp.h>
#include <sqlite3.h>
#include <stdlib.h>
#include <dlfcn.h>

extern lisp *lglobal;

static int ud_sql;
static void constructor(void) __attribute__((constructor));

static void ud_sql_free(cell *f) {
      /*if(!f->closed)
                sqlite3_close(userval(f));*/
        free(f);
}

static int ud_sql_print(io *o, unsigned depth, cell *f) {
        return printerf(NULL, o, depth, "%B<SQL-STATE:%d>%t", userval(f));
}

static int sql_callback(void *NotUsed, int argc, char **argv, char **azColName)
{       
        int i;
        (void)NotUsed;
        for (i = 0; i < argc; i++)
                printf("(%s %s)\n", azColName[i], argv[i] ? argv[i] : "NULL");
        printf("\n");
        return 0;
}

cell *subr_sql(lisp *l, cell *args) {
        char *zErrMsg;
        if(!cklen(args, 2) || !is_usertype(car(args), ud_sql) || !is_asciiz(CADR(args)))
               RECOVER(l, "\"expected (sql-database string)\" '%S", args); 
        if(sqlite3_exec(userval(car(args)), strval(CADR(args)), sql_callback, 0, &zErrMsg) != SQLITE_OK) {
                fprintf(stderr, "(sql-error \"%s\")\n", zErrMsg);
                sqlite3_free(zErrMsg);
        }
        return gsym_nil();
}

static void constructor(void) {
        ud_sql = newuserdef(lglobal, ud_sql_free, NULL, NULL, ud_sql_print);
        lisp_add_subr(lglobal, "sql", subr_sql);
}

