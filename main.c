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
 *  @todo None of the "subr" functions defined here handle errors in the best way
 *  @todo Experiment with auto insertion of parenthesis to make the shell
 *        easier to use, for example if an expression is missing right parens
 *        the line editing callback could add them in, or if there are more
 *        than one atoms on a line it could enclose them in parens and see if
 *        that can be evaluated "+ 2 2" would become "(+ 2 2)". This would only
 *        be part of the line editor.
 *  @todo There is an experimental folder, called "exp", which contains
 *        snippets of code and interfaces to libraries, such as libtcc, that
 *        functionality should be added in here.
 *  @todo Porting linedit to Windows would add functionality to this
 *        interpreter
 *  @todo Add standard unix functions, libtcc and regex support from the
 *        experimental section of the interpreter. Perhaps even OpenGL support.
 *        Unix functionality would include dlopen, directory handling, file
 *        stat, etc. Better still if generic Unix/Windows could be made.
 *  @todo A branch for the _Complex data type could be made, even if not
 *        maintained after the concept has been tested.
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

static cell *subr_hist_len(lisp *l, cell *args) {
        (void)l;
        if(!cklen(args, 1) || !isint(car(args)))
                return (cell*)mkerror();
        if(!line_history_set_maxlen((int)intval(car(args))))
                return (cell*)mkerror(); /*should really HALT the interpreter*/
        return (cell*)mktee();
}

static cell *subr_clear_screen(lisp *l, cell *args) {
        (void)l; (void)args;
        if(!cklen(args, 0)) return (cell*)mkerror();
        line_clearscreen();
        return (cell*)mktee();
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
        lisp_add_subr(l, subr_clear_screen,     "clear-screen");
        lisp_add_subr(l, subr_hist_len, "history-length");
#endif 
        return main_lisp_env(l, argc, argv); 
}

