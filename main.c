/** @file       main.c
 *  @brief      A minimal lisp interpreter and utility library, simple driver
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *
 *  See <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> for the
 *  license.
 *
 *  This file optionally adds functionality based on whether certain macros
 *  are defined, such as support for the functions in "math.h".
 *  
 *  Alternatively, the following code can be use to create a much simpler
 *  REPL without the extra functionality:
 *
 *      #include "liblisp.h"
 *      int main(int argc, char **argv) { return main_lisp(argc, argv); }
 * 
 *  @todo Some level of auto-completion could be added from the libline library
 *  @todo None of these functions handle errors in the best way
**/

#include "liblisp.h"

#ifdef USE_LINE
#include "libline/libline.h"
#include <string.h>
#include <stdlib.h>
static char *histfile = "hist.lsp";
#endif

#ifdef USE_MATH
#include <math.h>
/*@note It would be nice to add complex operations here as well, but
 *      that would require changes in the main interpreter*/

#define SUBR_MATH_UNARY(NAME)\
static cell *subr_ ## NAME (lisp *l, cell *args) {\
        if(!cklen(args, 1) || !isarith(car(args))) return (cell*)mkerror();\
        return mkfloat(l, NAME (isfloat(car(args)) ? floatval(car(args)) :\
                                  (double) intval(car(args))));\
}

#define MATH_UNARY_LIST\
        X(log)    X(log10)   X(fabs)    X(sin)     X(cos)   X(tan)\
        X(asin)   X(acos)    X(atan)    X(sinh)    X(cosh)  X(tanh)\
        X(exp)    X(sqrt)    X(ceil)    X(floor) /*X(log2)  X(round)\
        X(trunc)  X(tgamma)  X(lgamma)  X(erf)     X(erfc)*/

#define X(FUNC) SUBR_MATH_UNARY(FUNC)
MATH_UNARY_LIST
#undef X

static cell *subr_pow (lisp *l, cell *args) {
        cell *xo, *yo;
        double x, y;
        if(!cklen(args, 2) || !isarith(car(args)) || !isarith(car(cdr(args)))) 
                return (cell*)mkerror();
        xo = car(args);
        yo = car(cdr(args));
        x = isfloat(xo) ? floatval(xo) : intval(xo);
        y = isfloat(yo) ? floatval(yo) : intval(yo);
        return mkfloat(l, pow(x, y));
}

static cell *subr_modf(lisp *l, cell *args) {
        cell *xo;
        double x, fracpart, intpart = 0;
        if(!cklen(args, 1) || !isarith(car(args)))
                return (cell*)mkerror();
        xo = car(args);
        x = isfloat(xo) ? floatval(xo) : intval(xo);
        fracpart = modf(x, &intpart);
        return cons(l, mkfloat(l, intpart), mkfloat(l, fracpart));
}
#endif

#ifdef USE_LINE
static char *line_editing_function(const char *prompt) {
        char *line;
        line = line_editor(prompt);
        /*do not add blank lines*/
        if(!line || !line[strspn(line, " \t\r\n")]) return line;
        line_history_add(line);
        line_history_save(histfile);
        return line;
}

static cell *subr_line_editor_mode(lisp *l, cell *args) {
        (void)l;
        if(cklen(args, 1)) { 
                line_set_vi_mode(isnil(car(args)) ? 0 : 1);
                return (cell*)mktee();
        }
        return line_get_vi_mode() ? (cell*)mktee(): (cell*)mknil();
}
#endif

int main(int argc, char **argv) { 
        lisp *l = lisp_init();
        if(!l) return PRINT_ERROR("\"%s\"", "initialization failed"), -1;

#ifdef USE_MATH /*add math functionality from math.h*/
#define X(FUNC) lisp_add_subr(l, subr_ ## FUNC, # FUNC);
MATH_UNARY_LIST
#undef X
        lisp_add_subr(l, subr_pow, "pow");
        lisp_add_subr(l, subr_modf, "modf");
#endif

#ifdef USE_LINE /*add line editor functionality*/
        lisp_set_line_editor(l, line_editing_function);
        line_history_load(histfile);
        line_set_vi_mode(1); /*start up in a sane editing mode*/
        lisp_add_subr(l, subr_line_editor_mode, "line-editor-mode");
#endif 
        return main_lisp_env(l, argc, argv); 
}

