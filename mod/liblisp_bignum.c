/** @file       liblisp_bignum.c
 *  @brief      arbitrary precision arithmetic module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com 
 *  @bug        There are possible memory leaks in bignum.c or the interaction
 *              between bignum.c and other things in the interpreter. Also,
 *              invalid bignums or strings to the bignum.c are not handled
 *              correctly.
 *  **/
#include <assert.h>
#include <liblisp.h>
#include <stdlib.h>
#include <stdint.h>
#include "bignum.h"

#define SUBROUTINE_XLIST\
	X("bignum",             subr_bignum_create,     "d", "create a bignum from an integer")\
	X("bignum-multiply",    subr_bignum_multiply,   NULL, "multiply two bignums")\
	X("bignum-add",         subr_bignum_add,        NULL, "add two bignums")\
	X("bignum-subtract",    subr_bignum_subtract,   NULL, "subtract one bignum from another")\
	X("bignum-divide",      subr_bignum_divide,     NULL, "bignum division")\
	X("bignum-to-string",   subr_bignum_to_string,  NULL, "convert a bignum to a string")\
	X("bignum-from-string", subr_bignum_from_string, "S", "create a bignum from a string")

#define X(NAME, SUBR, VALIDATION , DOCSTRING) static cell* SUBR (lisp *l, cell *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(NAME, DOCSTRING), SUBR },
static struct module_subroutines {
	char *name, *validate, *docstring;
	subr p;
} primitives[] = {
	SUBROUTINE_XLIST	/*all of the subr functions */
	{ NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};

#undef X

static int ud_bignum = 0;

static void ud_bignum_free(cell * f)
{
	bignum_destroy(get_user(f));
	free(f);
}

static int ud_bignum_print(io * o, unsigned depth, cell * f)
{
	int ret;
	char *s;
	s = bignum_bigtostr(get_user(f), 10);
	ret = lisp_printf(NULL, o, depth, "%m{bignum:%s}%t", s);
	free(s);
	return ret;
}

static cell *subr_bignum_create(lisp * l, cell * args)
{
	bignum *b;
	if (!(b = bignum_create(get_int(car(args)), 16)))
		HALT(l, "\"%s\"", "out of memory");
	return mk_user(l, b, ud_bignum);
}

static cell *subr_bignum_multiply(lisp * l, cell * args)
{
	bignum *b;
	if (!cklen(args, 2) || !is_usertype(car(args), ud_bignum) || !is_usertype(CADR(args), ud_bignum))
		RECOVER(l, "\"expected (bignum bignum)\" '%S", args);
	if (!(b = bignum_multiply(get_user(car(args)), get_user(CADR(args)))))
		HALT(l, "\"%s\"", "out of memory");
	return mk_user(l, b, ud_bignum);
}

static cell *subr_bignum_add(lisp * l, cell * args)
{
	bignum *b;
	if (!cklen(args, 2) || !is_usertype(car(args), ud_bignum) || !is_usertype(CADR(args), ud_bignum))
		RECOVER(l, "\"expected (bignum bignum)\" '%S", args);
	if (!(b = bignum_add(get_user(car(args)), get_user(CADR(args)))))
		HALT(l, "\"%s\"", "out of memory");
	return mk_user(l, b, ud_bignum);
}

static cell *subr_bignum_subtract(lisp * l, cell * args)
{
	bignum *b;
	if (!cklen(args, 2) || !is_usertype(car(args), ud_bignum) || !is_usertype(CADR(args), ud_bignum))
		RECOVER(l, "\"expected (bignum bignum)\" '%S", args);
	if (!(b = bignum_subtract(get_user(car(args)), get_user(CADR(args)))))
		HALT(l, "\"%s\"", "out of memory");
	return mk_user(l, b, ud_bignum);
}

static cell *subr_bignum_divide(lisp * l, cell * args)
{
	bignum_div_t *d;
	cell *ret;
	if (!cklen(args, 2) || !is_usertype(car(args), ud_bignum) || !is_usertype(CADR(args), ud_bignum))
		RECOVER(l, "\"expected (bignum bignum)\" '%S", args);
	if (!(d = bignum_divide(get_user(car(args)), get_user(CADR(args))), ud_bignum))
		HALT(l, "\"%s\"", "out of memory");
	ret = cons(l, mk_user(l, d->quotient, ud_bignum), mk_user(l, d->remainder, ud_bignum));
	free(d);
	return ret;
}

static cell *subr_bignum_to_string(lisp * l, cell * args)
{
	char *s;
	if (!cklen(args, 1) || !is_usertype(car(args), ud_bignum))
		RECOVER(l, "\"expected (bignum)\" '%S", args);
	if (!(s = bignum_bigtostr(get_user(car(args)), 10)))
		HALT(l, "\"%s\"", "out of memory");
	return mk_str(l, s);
}

static cell *subr_bignum_from_string(lisp * l, cell * args)
{
	bignum *b;
	if (!(b = bignum_strtobig(get_str(car(args)), 10)))
		HALT(l, "\"%s\"", "out of memory");
	return mk_user(l, b, ud_bignum);
}

static int initialize(void)
{
	size_t i;
	assert(lglobal);

	ud_bignum = new_user_defined_type(lglobal, ud_bignum_free, NULL, NULL, ud_bignum_print);
	if (ud_bignum < 0)
		goto fail;
	for (i = 0; primitives[i].p; i++)	/*add all primitives from this module */
		if (!lisp_add_subr(lglobal, primitives[i].name, primitives[i].p, primitives[i].validate, primitives[i].docstring))
			goto fail;
	if (lisp_verbose_modules)
		lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: bignum loaded\n");
	return 0;
 fail:	lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: bignum load failure\n");
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
