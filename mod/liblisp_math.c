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
	
/*more to come*/
/*X(isnan,  "f", "is float NaN?")\ */
/*X(hypot,  "a a", "sqrt(x*x + y*y)")\ */


#define X(FUNC, VALIDATION, DOCSTRING) SUBR_MATH_UNARY(FUNC, VALIDATION, DOCSTRING)
MATH_UNARY_LIST
#undef X

#define X(SUBR, VALIDATION, DOCSTRING) { # SUBR, VALIDATION, MK_DOCSTR( #SUBR, DOCSTRING), subr_ ## SUBR },
static lisp_module_subroutines_t primitives[] = {
	MATH_UNARY_LIST
	{ NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};
#undef X

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
