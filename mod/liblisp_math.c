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
#define SUBR_MATH_UNARY(NAME)\
static cell *subr_ ## NAME (lisp *l, cell *args) {\
	VALIDATE(l, 1, "A", args, 1);\
        return mk_float(l, NAME (is_floatval(car(args)) ? floatval(car(args)) :\
                                  (double) intval(car(args))));\
}

#define MATH_UNARY_LIST\
        X(log)    X(log10)   X(fabs)    X(sin)     X(cos)   X(tan)\
        X(asin)   X(acos)    X(atan)    X(sinh)    X(cosh)  X(tanh)\
        X(exp)    X(sqrt)    X(ceil)    X(floor)

#define X(FUNC) SUBR_MATH_UNARY(FUNC)
MATH_UNARY_LIST
#undef X

static cell *subr_pow (lisp *l, cell *args) {
        cell *xo, *yo;
        double x, y;
	VALIDATE(l, 2, "A A", args, 1);
        xo = car(args);
        yo = CADR(args);
        x = is_floatval(xo) ? floatval(xo) : intval(xo);
        y = is_floatval(yo) ? floatval(yo) : intval(yo);
        return mk_float(l, pow(x, y));
}

static cell *subr_modf(lisp *l, cell *args) {
        cell *xo;
        double x, fracpart, intpart = 0;
	VALIDATE(l, 1, "A", args, 1);
        xo = car(args);
        x = is_floatval(xo) ? floatval(xo) : intval(xo);
        fracpart = modf(x, &intpart);
        return cons(l, mk_float(l, intpart), mk_float(l, fracpart));
}

#define X(SUBR) { subr_ ## SUBR, # SUBR },
static struct module_subroutines { subr p; char *name; } primitives[] = {
        MATH_UNARY_LIST /*all of the subr functions*/
        { subr_modf, "modf" },
        { subr_pow,  "pow" },
        { NULL, NULL} /*must be terminated with NULLs*/
};
#undef X

static int initialize(void) {
        size_t i;
        assert(lglobal);

        for(i = 0; primitives[i].p; i++) /*add all primitives from this module*/
                if(!lisp_add_subr(lglobal, primitives[i].name, primitives[i].p))
                        goto fail;
        lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: math loaded\n");
        return 0;
fail:   lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: math load failure\n");
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__((constructor));
static void destruct(void)  __attribute__((destructor));
static void construct(void) { initialize(); }
static void destruct(void)  { }
#elif _WIN32
#include <windows.h>
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
        switch (fdwReason) {
            case DLL_PROCESS_ATTACH: initialize(); break;
            case DLL_PROCESS_DETACH: break;
            case DLL_THREAD_ATTACH:  break;
            case DLL_THREAD_DETACH:  break;
	    default: break;
        }
        return TRUE;
}
#endif


