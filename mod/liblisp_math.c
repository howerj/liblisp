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

extern lisp *lglobal; /* from main.c */

static void construct(void) __attribute__((constructor));
static void destruct(void) __attribute__((destructor));

/**@brief Template for most of the functions in "math.h"
 * @param NAME name of math function such as "log", "sin", etc.*/
#define SUBR_MATH_UNARY(NAME)\
static cell *subr_ ## NAME (lisp *l, cell *args) {\
        if(!cklen(args, 1) || !is_arith(car(args)))\
                RECOVER(l, "\"expected (number)\" '%S", args);\
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
        if(!cklen(args, 2) || !is_arith(car(args)) || !is_arith(CADR(args)))
                RECOVER(l, "\"expected (number number)\" '%S", args);
        xo = car(args);
        yo = CADR(args);
        x = is_floatval(xo) ? floatval(xo) : intval(xo);
        y = is_floatval(yo) ? floatval(yo) : intval(yo);
        return mk_float(l, pow(x, y));
}

static cell *subr_modf(lisp *l, cell *args) {
        cell *xo;
        double x, fracpart, intpart = 0;
        if(!cklen(args, 1) || !is_arith(car(args)))
                RECOVER(l, "\"expected (number)\" '%S", args);
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

static void construct(void) {
        size_t i;
        assert(lglobal);

        for(i = 0; primitives[i].p; i++) /*add all primitives from this module*/
                if(!lisp_add_subr(lglobal, primitives[i].name, primitives[i].p))
                        goto fail;
        lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: math loaded\n");
        return;
fail:   lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: math load failure\n");
}

static void destruct(void) {
}
