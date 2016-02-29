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
#include <sqlite3.h>
#include <stdlib.h>

static int ud_sql;

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

static int sql_callback(void *obin, int argc, char **argv, char **azColName)
{
	int i;
	cell *ob;
	assert(obin);
	ob = *((cell **) obin);
	for (i = 0; i < argc; i++)
		ob = cons(lglobal,
			  cons(lglobal, mk_str(lglobal, lstrdup(azColName[i])), argv[i] ? mk_str(lglobal, lstrdup(argv[i])) : gsym_nil()), ob);
	*((cell **) obin) = ob;
	return 0;
}

static cell *subr_sqlopen(lisp * l, cell * args)
{
	sqlite3 *db;
	assert(l == lglobal);
	VALIDATE(l, "subr_sqlopen", 1, "Z", args, 1);
	if (sqlite3_open(get_str(car(args)), &db)) {
		lisp_printf(l, lisp_get_logging(l), 0, "(sql-error \"%s\")\n", sqlite3_errmsg(db));
		sqlite3_close(db);
		return gsym_error();
	}
	return mk_user(l, db, ud_sql);
}

static cell *subr_sql(lisp * l, cell * args)
{
	char *errmsg;
	cell *head = gsym_nil();
	assert(l == lglobal);
	if (!cklen(args, 2) || !is_usertype(car(args), ud_sql) || !is_asciiz(CADR(args)))
		RECOVER(l, "\"expected (sql-database string)\" '%S", args);
	if (sqlite3_exec(get_user(car(args)), get_str(CADR(args)), sql_callback, &head, &errmsg) != SQLITE_OK) {
		lisp_printf(l, lisp_get_logging(l), 0, "(sql-error \"%s\")\n", errmsg);
		sqlite3_free(errmsg);
		return gsym_error();
	}
	return head;
}

static cell *subr_sqlclose(lisp * l, cell * args)
{
	assert(l == lglobal);
	if (!cklen(args, 1) || !is_usertype(car(args), ud_sql))
		RECOVER(l, "\"expected (sql-database)\" '%S", args);
	sqlite3_close(get_user(car(args)));
	return gsym_tee();
}

static int initialize(void)
{
	assert(lglobal);
	ud_sql = new_user_defined_type(lglobal, ud_sql_free, NULL, NULL, ud_sql_print);
	if (ud_sql < 0)
		goto fail;
	if (!lisp_add_subr(lglobal, "sql", subr_sql, NULL, NULL))
		goto fail;
	if (!lisp_add_subr(lglobal, "sql-open", subr_sqlopen, NULL, NULL))
		goto fail;
	if (!lisp_add_subr(lglobal, "sql-close", subr_sqlclose, NULL, NULL))
		goto fail;
	if (lisp_verbose_modules)
		lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: sqlite3 loaded\n");
	return 0;
 fail:	lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: sqlite3 load failure\n");
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__ ((constructor));
static void destruct(void) __attribute__ ((destructor));
static void construct(void)
{
	initialize();
}

static void destruct(void)
{
}
#elif _WIN32
#include <windows.h>
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNUSED(hinstDLL);
	UNUSED(lpvReserved);
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		initialize();
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
