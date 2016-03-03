/** @file       liblisp_math.c
 *  @brief      Basic mathematical functions for module for liblisp
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com**/
#include <liblisp.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

/**@brief Template for most of the functions in "math.h"
 * @param NAME name of math function such as "log", "sin", etc.*/
#define SUBR_MATH_UNARY(NAME, VALIDATION, DOCSTRING)\
static cell *subr_ ## NAME (lisp *l, cell *args) {\
        return mk_float(l, NAME (get_a2f(car(args))));\
}

#define MATH_UNARY_LIST\
	X(log,   "a", "natural logarithm")\
	X(fabs,  "a", "absolute value")\
	X(sin,   "a", "sine")\
	X(cos,   "a", "cosine")\
	X(tan,   "a", "tangent")\
	X(asin,  "a", "arcsine")\
	X(acos,  "a", "arcosine")\
	X(atan,  "a", "arctangent")\
	X(sinh,  "a", "hyperbolic sine")\
	X(cosh,  "a", "hyperbolic cosine")\
	X(tanh,  "a", "hyperbolic tangent")\
	X(exp,   "a", "exponential function")\
	X(sqrt,  "a", "square root")\
	X(ceil,  "a", "ceiling")\
	X(floor, "a", "floor")\
	X(log10, "a", "logarithm (base 10)")

#define X(FUNC, VALIDATION, DOCSTRING) SUBR_MATH_UNARY(FUNC, VALIDATION, DOCSTRING)
MATH_UNARY_LIST
#undef X
static cell *subr_pow(lisp * l, cell * args)
{
	return mk_float(l, pow(get_a2f(car(args)), get_a2f(CADR(args))));
}

static cell *subr_modf(lisp * l, cell * args)
{
	double x, fracpart, intpart = 0;
	x = get_a2f(car(args));
	fracpart = modf(x, &intpart);
	return cons(l, mk_float(l, intpart), mk_float(l, fracpart));
}

#define X(SUBR, VALIDATION, DOCSTRING) { # SUBR, VALIDATION, MK_DOCSTR( #SUBR, DOCSTRING), subr_ ## SUBR },
static struct module_subroutines {
	char *name, *validate, *docstring;
	subr p;
} primitives[] = {
	MATH_UNARY_LIST		/*all of the subr functions */
	{ "modf", "a", MK_DOCSTR("modf", "split a float into integer and fractional parts"), subr_modf}, 
	{ "pow", "a a", MK_DOCSTR("pow:", "raise a base to a power"), subr_pow}, 
	{ NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};

#undef X

int lisp_module_initialize(lisp *l)
{
	size_t i;
	assert(l);

	for (i = 0; primitives[i].p; i++)	/*add all primitives from this module */
		if (!lisp_add_subr(l, primitives[i].name, primitives[i].p, primitives[i].validate, primitives[i].docstring))
			goto fail;
	if (lisp_verbose_modules)
		lisp_printf(l, lisp_get_logging(l), 0, "module: math loaded\n");
	return 0;
 fail:	lisp_printf(l, lisp_get_logging(l), 0, "module: math load failure\n");
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
