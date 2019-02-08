/** @file       liblisp_math.c
 *  @brief      c99 mathematical functions for module for liblisp
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html>
 *  @email      howe.r.j.89@gmail.com**/
#include <lispmod.h>
#include <assert.h>
#include <math.h>
#include <stdlib.h>

#define SUBR_MATH_UNARY(NAME, VALIDATION, DOCSTRING)\
static lisp_cell_t *subr_ ## NAME (lisp_t *l, lisp_cell_t *args) {\
        return mk_float(l, NAME (get_a2f(car(args))));\
}

#define MATH_UNARY_LIST\
	X(erf,    "a", "computes error function")\
	X(erfc,   "a", "computes complementary error function")\
	X(tgamma, "a", "computes gamma function")\
	X(lgamma, "a", "computes natural logarithm of gamma function")\
	X(expm1,  "a", "computes (e^x)-1")\
	X(exp2,   "a", "computes 2^x")\
	X(log2,   "a", "computes base-2 logarithm")\
	X(log1p,  "a", "computes ln(1+x)")\
	X(cbrt,   "a", "computes cubic root")\
	X(asinh,  "a", "computes inverse hyperbolic sine")\
	X(acosh,  "a", "computes inverse hyperbolic cosine")\
	X(atanh,  "a", "computes inverse hyperbolic tangent")\
	X(trunc,  "a", "rounds to nearest integer not greater in magnitude than given value")\
	X(round,  "a", "rounds to nearest integer, rounding away from zero in halfway cases")

#define SUBROUTINE_XLIST\
        X("nan?",  subr_isnan,   "f", "Is this float a NaN float?")\
        X("infinite?",  subr_isinf,   "f", "Is this float a Infinite float?")\
	X("hypot", subr_hypot, "a a", "Computes the square root of the sum of the squares of two numbers")\
	X("fma",   subr_fma, "a a a", "Computes fused-multiply-add")\
	X("unordered?", subr_unordered, "f f", "Are two floats unordered?")\
	X("finite?", subr_finite, "f", "Is this float finite?")

#define X(FUNC, VALIDATION, DOCSTRING) SUBR_MATH_UNARY(FUNC, VALIDATION, DOCSTRING)
MATH_UNARY_LIST
#undef X

#define X(NAME, SUBR, VALIDATION, DOCSTRING) static lisp_cell_t * SUBR (lisp_t *l, lisp_cell_t *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X

static lisp_module_subroutines_t primitives[] = {
#define X(SUBR, VALIDATION, DOCSTRING) { # SUBR, VALIDATION, MK_DOCSTR( #SUBR, DOCSTRING), subr_ ## SUBR },
	MATH_UNARY_LIST
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(#SUBR, DOCSTRING), SUBR },
	SUBROUTINE_XLIST
#undef X
	{ NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};

static lisp_cell_t *subr_finite(lisp_t *l, lisp_cell_t *args)
{
	UNUSED(l);
	double a = get_float(car(args));
	return isfinite(a) ? gsym_tee() : gsym_nil();
}

static lisp_cell_t *subr_unordered(lisp_t *l, lisp_cell_t *args)
{
	UNUSED(l);
	double a = get_float(car(args)), b = get_float(CADR(args));
	return isunordered(a, b) ? gsym_tee() : gsym_nil();
}

static lisp_cell_t *subr_isnan(lisp_t *l, lisp_cell_t *args)
{
	UNUSED(l);
	return isnan(get_float(car(args))) ? gsym_tee() : gsym_nil();
}

static lisp_cell_t *subr_isinf(lisp_t *l, lisp_cell_t *args)
{
	UNUSED(l);
	return isinf(get_float(car(args))) ? gsym_tee() : gsym_nil();
}

static lisp_cell_t *subr_hypot(lisp_t *l, lisp_cell_t *args)
{
	return mk_float(l, hypot(get_a2f(car(args)), get_a2f(CADR(args))));
}

static lisp_cell_t *subr_fma(lisp_t *l, lisp_cell_t *args)
{
	return mk_float(l, fma(get_a2f(car(args)), get_a2f(CADR(args)), get_a2f(CADDR(args))));
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
