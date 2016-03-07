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

#define X(NAME, SUBR, VALIDATION , DOCSTRING) static lisp_cell_t * SUBR (lisp_t *l, lisp_cell_t *args);
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

static void ud_bignum_free(lisp_cell_t * f)
{
	bignum_destroy(get_user(f));
	free(f);
}

static int ud_bignum_print(io_t * o, unsigned depth, lisp_cell_t * f)
{
	int ret;
	char *s;
	s = bignum_bigtostr(get_user(f), 10);
	ret = lisp_printf(NULL, o, depth, "%m{bignum:%s}%t", s);
	free(s);
	return ret;
}

static lisp_cell_t *subr_bignum_create(lisp_t * l, lisp_cell_t * args)
{
	bignum *b;
	if (!(b = bignum_create(get_int(car(args)), 16)))
		LISP_HALT(l, "\"%s\"", "out of memory");
	return mk_user(l, b, ud_bignum);
}

static lisp_cell_t *subr_bignum_multiply(lisp_t * l, lisp_cell_t * args)
{
	bignum *b;
	if (!cklen(args, 2) || !is_usertype(car(args), ud_bignum) || !is_usertype(CADR(args), ud_bignum))
		LISP_RECOVER(l, "\"expected (bignum bignum)\" '%S", args);
	if (!(b = bignum_multiply(get_user(car(args)), get_user(CADR(args)))))
		LISP_HALT(l, "\"%s\"", "out of memory");
	return mk_user(l, b, ud_bignum);
}

static lisp_cell_t *subr_bignum_add(lisp_t * l, lisp_cell_t * args)
{
	bignum *b;
	if (!cklen(args, 2) || !is_usertype(car(args), ud_bignum) || !is_usertype(CADR(args), ud_bignum))
		LISP_RECOVER(l, "\"expected (bignum bignum)\" '%S", args);
	if (!(b = bignum_add(get_user(car(args)), get_user(CADR(args)))))
		LISP_HALT(l, "\"%s\"", "out of memory");
	return mk_user(l, b, ud_bignum);
}

static lisp_cell_t *subr_bignum_subtract(lisp_t * l, lisp_cell_t * args)
{
	bignum *b;
	if (!cklen(args, 2) || !is_usertype(car(args), ud_bignum) || !is_usertype(CADR(args), ud_bignum))
		LISP_RECOVER(l, "\"expected (bignum bignum)\" '%S", args);
	if (!(b = bignum_subtract(get_user(car(args)), get_user(CADR(args)))))
		LISP_HALT(l, "\"%s\"", "out of memory");
	return mk_user(l, b, ud_bignum);
}

static lisp_cell_t *subr_bignum_divide(lisp_t * l, lisp_cell_t * args)
{
	bignum_div_t *d;
	lisp_cell_t *ret;
	if (!cklen(args, 2) || !is_usertype(car(args), ud_bignum) || !is_usertype(CADR(args), ud_bignum))
		LISP_RECOVER(l, "\"expected (bignum bignum)\" '%S", args);
	if (!(d = bignum_divide(get_user(car(args)), get_user(CADR(args))), ud_bignum))
		LISP_HALT(l, "\"%s\"", "out of memory");
	ret = cons(l, mk_user(l, d->quotient, ud_bignum), mk_user(l, d->remainder, ud_bignum));
	free(d);
	return ret;
}

static lisp_cell_t *subr_bignum_to_string(lisp_t * l, lisp_cell_t * args)
{
	char *s;
	if (!cklen(args, 1) || !is_usertype(car(args), ud_bignum))
		LISP_RECOVER(l, "\"expected (bignum)\" '%S", args);
	if (!(s = bignum_bigtostr(get_user(car(args)), 10)))
		LISP_HALT(l, "\"%s\"", "out of memory");
	return mk_str(l, s);
}

static lisp_cell_t *subr_bignum_from_string(lisp_t * l, lisp_cell_t * args)
{
	bignum *b;
	if (!(b = bignum_strtobig(get_str(car(args)), 10)))
		LISP_HALT(l, "\"%s\"", "out of memory");
	return mk_user(l, b, ud_bignum);
}

int lisp_module_initialize(lisp_t *l)
{
	size_t i;
	assert(l);

	/**@bug ud_bignum needs to be on a per lisp interpreter basis*/
	ud_bignum = new_user_defined_type(l, ud_bignum_free, NULL, NULL, ud_bignum_print);
	if (ud_bignum < 0)
		goto fail;
	for (i = 0; primitives[i].p; i++)	/*add all primitives from this module */
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
static void destruct(void) {}
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
