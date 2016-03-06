/** @file       liblisp_sql.c
 *  @brief      SQL (sqlite3) interface module for liblisp
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *  
 *  @todo Improve interface, add prepared statements, ...**/
#include <assert.h>
#include <liblisp.h>
#include <sqlite3.h> /*see https://www.sqlite.org*/
#include <stdlib.h>

typedef struct {
	lisp *l;
	cell *ret;
} sqlite3_cb_t;

static int ud_sql;

#define SUBROUTINE_XLIST\
	X("sql",         subr_sql,        NULL, "Execute an SQL statement given an SQLite3 database handle and a statement string")\
	X("sql-open",    subr_sql_open,   "Z",  "Open an SQLite3 database file")\
	X("sql-close",   subr_sql_close,  NULL, "Close an SQLite3 database handle")\
	X("sql-info",    subr_sql_info,   "",   "Return version information about the SQL library")\
	X("sql-is-thread-safe?",    subr_sql_is_thread_safe,  "",  "Is the SQlite3 thread safe?")\

#define X(NAME, SUBR, VALIDATION , DOCSTRING) static cell* SUBR (lisp *l, cell *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(NAME, DOCSTRING), SUBR },
static struct module_subroutines {
	char *name, *validate, *docstring;
	subr p;
} primitives[] = {
	SUBROUTINE_XLIST		/*all of the subr functions */
	{ NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};

static void ud_sql_free(cell * f)
{
	if (!is_closed(f))
		sqlite3_close(get_user(f));
	free(f);
}

static int ud_sql_print(io * o, unsigned depth, cell * f)
{
	return lisp_printf(NULL, o, depth, "%B<SQL-STATE:%d:%s>%t", get_user(f), is_closed(f) ? "CLOSED" : "OPEN");
}

static cell *subr_sql_open(lisp * l, cell * args)
{
	sqlite3 *db;
	if (sqlite3_open(get_str(car(args)), &db)) {
		lisp_printf(l, lisp_get_logging(l), 0, "(sql-error \"%s\")\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return gsym_error();
	}
	return mk_user(l, db, ud_sql);
}

static cell *subr_sql_close(lisp * l, cell * args)
{
	if (!cklen(args, 1) || !is_usertype(car(args), ud_sql))
		LISP_RECOVER(l, "\"expected (sql-database)\" '%S", args);
	sqlite3_close(get_user(car(args)));
	close_cell(car(args));
	return gsym_tee();
}

/** @todo This callback is probably a little bit too verbose, it puts the field
 * names in all of the entries, but this *might* not be necessary, instead it
 * could be done only once, perhaps*/
static int sql_callback(void *obin, int argc, char **argv, char **azColName)
{
	int i;
	assert(obin);
	sqlite3_cb_t *cb = (sqlite3_cb_t *)obin;
	cell *cur = gsym_nil();
	cell *list = cb->ret;
	lisp *l = cb->l;
	for (i = 0; i < argc; i++)
		cur = cons(l,
			  cons(l, 
				  mk_str(l, lstrdup_or_abort(azColName[i])), 
				  	argv[i] ? 
						mk_str(l, lstrdup_or_abort(argv[i])) : 
						gsym_nil()), 
			  	cur);
	cb->ret = cons(l, cur, list);
	return 0;
}

static cell *subr_sql(lisp * l, cell * args)
{
	intptr_t rc = 0;
	char *errmsg = NULL;
	sqlite3_cb_t cb;
	cb.ret = gsym_nil();
	cb.l   = l;
	if (!cklen(args, 2) || !is_usertype(car(args), ud_sql) || !is_asciiz(CADR(args)))
		LISP_RECOVER(l, "\"expected (sql-database string)\" '%S", args);
	if ((rc = sqlite3_exec(get_user(car(args)), get_str(CADR(args)), sql_callback, &cb, &errmsg)) != SQLITE_OK) {
		cell *r;
		r = mk_list(l, gsym_error(), mk_str(l, lisp_strdup(l, errmsg)), mk_int(l, rc), NULL);
		sqlite3_free(errmsg);
		return r;
	}
	return cb.ret;
}

static cell *subr_sql_info(lisp *l, cell *args)
{
	UNUSED(args);
	return mk_list(l,
		mk_immutable_str(l, "sqlite3"),
		mk_immutable_str(l, sqlite3_libversion()), 
		mk_immutable_str(l, sqlite3_sourceid()), NULL);
}

static cell *subr_sql_is_thread_safe(lisp *l, cell *args)
{
	UNUSED(l);
	UNUSED(args);
	return sqlite3_threadsafe() ? gsym_tee() : gsym_nil();
}

int lisp_module_initialize(lisp *l)
{
	assert(l);
	if(sqlite3_os_init() != SQLITE_OK)
		goto fail;
	/**@bug ud_sql needs to be on a per lisp thread basis*/
	ud_sql = new_user_defined_type(l, ud_sql_free, NULL, NULL, ud_sql_print);
	if (ud_sql < 0)
		goto fail;
	for (size_t i = 0; primitives[i].p; i++)	/*add all primitives from this module */
		if (!lisp_add_subr(l, primitives[i].name, primitives[i].p, primitives[i].validate, primitives[i].docstring))
			goto fail;

	return 0;
 fail:	
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__ ((constructor));
static void destruct(void) __attribute__ ((destructor));
static void construct(void) {}
static void destruct(void) { sqlite3_shutdown(); }
#elif _WIN32
#include <windows.h>
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNUSED(hinstDLL);
	UNUSED(lpvReserved);
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}
#endif
