/** @file       liblisp_diff.c
 *  @brief      diff util module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html>
 *  @email      howe.r.j.89@gmail.com
 *  @bug        This is very buggy!**/

#include "diff.h"
#include "tsort.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <lispmod.h>

#define SUBROUTINE_XLIST\
        X("diff",  subr_diff,  "c c", "print the diff of two lists of strings")\
        X("tsort", subr_tsort, "",    "perform a topological sort on a list of dependencies")
	/*strstr, strerr (move from subr.c), strpbrk, strrchr, strspn, thread
	 * safe strtok, ...*/

#define X(NAME, SUBR, VALIDATION , DOCSTRING) static lisp_cell_t * SUBR (lisp_t *l, lisp_cell_t *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(NAME, DOCSTRING), SUBR },
static lisp_module_subroutines_t primitives[] = {
	SUBROUTINE_XLIST	/*all of the subr functions */
	{NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};

#undef X

static lisp_cell_t *make_diff_inner(lisp_t * l, diff * d, char *x[], char *y[], size_t i, size_t j, lisp_cell_t * pp, lisp_cell_t * mm)
{ /*lol*/
	if (i > 0 && j > 0 && !strcmp(x[i - 1], y[j - 1])) {
		return cons(l, cons(l, mk_str(l, lisp_strdup(l, x[i - 1])), gsym_nil()), make_diff_inner(l, d, x, y, i - 1, j - 1, pp, mm));
	} else if (j > 0 && (i == 0 || d->c[(i * (d->n)) + (j - 1)] >= d->c[((i - 1) * (d->n)) + j])) {
		return cons(l, cons(l, pp, cons(l, mk_str(l, lisp_strdup(l, y[j - 1])), gsym_nil())), make_diff_inner(l, d, x, y, i, j - 1, pp, mm));
	} else if (i > 0 && (j == 0 || d->c[(i * (d->n)) + (j - 1)] < d->c[((i - 1) * (d->n)) + j])) {
		return cons(l, cons(l, mm, cons(l, mk_str(l, lisp_strdup(l, x[i - 1])), gsym_nil())), make_diff_inner(l, d, x, y, i - 1, j, pp, mm));
	}
	return gsym_nil();
}

static lisp_cell_t *make_diff(lisp_t * l, diff * d, char **x, char **y)
{
	assert(l && d && x && y);
	lisp_cell_t *pp = lisp_intern(l, "+"), *mm = lisp_intern(l, "-");
	return make_diff_inner(l, d, x, y, d->m, d->n, pp, mm);
}

static lisp_cell_t *subr_diff(lisp_t * l, lisp_cell_t * args)
{ 	/**@bug makes results in reverse order*/
	/**@todo This should operate on two strings as well as lists of strings*/
	size_t i;
	lisp_cell_t *a, *b, *tmp, *ret = NULL;
	char **aa, **bb;
	diff *d = NULL;
	a = car(args);
	b = CADR(args);
	if (!(aa = calloc(get_length(a) + 2, sizeof(*aa))))  /**@bug +2??*/
		LISP_HALT(l, "\"%s\"", "out of memory");
	if (!(bb = calloc(get_length(b) + 2, sizeof(*bb))))  /**@bug +2??*/
		LISP_HALT(l, "\"%s\"", "out of memory");
	for (tmp = a, i = 0; !is_nil(tmp); tmp = cdr(tmp), i++)
		if (!is_asciiz(car(tmp)))
			goto cleanup;
		else
			aa[i] = get_str(car(tmp));
	for (tmp = b, i = 0; !is_nil(tmp); tmp = cdr(tmp), i++)
		if (!is_asciiz(car(tmp)))
			goto cleanup;
		else
			bb[i] = get_str(car(tmp));
	if (!(d = lcs(aa, get_length(a), bb, get_length(b))))
		LISP_HALT(l, "\"%s\"", "out of memory");
	ret = make_diff(l, d, aa, bb);
 cleanup:
	free(aa);
	free(bb);
	if (d)
		free(d->c);
	free(d);
	if (ret)
		return ret;
	LISP_RECOVER(l, "\"expected two lists of strings\" '%S", args);
	return gsym_error();
}

static lisp_cell_t *subr_tsort(lisp_t * l, lisp_cell_t * args)
{/**@todo implement me!*/
	UNUSED(l);
	UNUSED(args);
	return gsym_nil();
}

int lisp_module_initialize(lisp_t *l)
{
	assert(l);
	if(lisp_add_module_subroutines(l, primitives, 0) < 0)
		goto fail;
	return 0;
 fail:
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__ ((constructor));
static void destruct(void) __attribute__ ((destructor));
static void construct(void) { }
static void destruct(void) { }
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
