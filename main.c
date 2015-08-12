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
 *  @todo Experiment with auto insertion of parenthesis to make the shell
 *        easier to use, for example if an expression is missing right parens
 *        the line editing callback could add them in, or if there are more
 *        than one atoms on a line it could enclose them in parens and see if
 *        that can be evaluated "+ 2 2" would become "(+ 2 2)". This would only
 *        be part of the line editor.
 *  @todo Porting libline to Windows
**/

#include "liblisp.h"
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

#ifndef VERSION
#define VERSION unknown    /**< Version of the interpreter*/
#endif

#ifndef VCS_COMMIT
#define VCS_COMMIT unknown /**< Version control commit of this interpreter*/
#endif

#ifndef VCS_ORIGIN
#define VCS_ORIGIN unknown /**< Version control repository origin*/
#endif

static lisp *lglobal; /**< used for signal handler*/
static int running; /**< only handle errors when the lisp interpreter is running*/

/** @brief This function tells a running lisp REPL to halt if it is reading
 *         input or printing output, but if it is evaluating an expression
 *         instead it will stop doing that and try to get more input. This is
 *         so the user can sent the SIGINT signal to the interpreter to halt
 *         expressions that are not terminating, but can still use SIGINT to
 *         halt the interpreter by at maximum sending the signal twice. This
 *         function is meant to be passed to "signal".
 *  @param sig This will only be SIGINT in our case. */
static void sig_int_handler(int sig) {
        if(!running || !lglobal) exit(0);  /*exit if lisp environment is not running*/
        lisp_set_signal(lglobal, sig); /*notify lisp environment of signal*/
        running = 0;
}

#ifdef USE_LINE
#include "libline/libline.h"
#include <string.h>
#include <stdlib.h>
static char *histfile = ".list";
#endif

#include <math.h>

/**@brief
 * @param NAME*/
#define SUBR_MATH_UNARY(NAME)\
static cell *subr_ ## NAME (lisp *l, cell *args) {\
        if(!cklen(args, 1) || !isarith(car(args)))\
                RECOVER(l, "\"expected (number)\" '%S", args);\
        return mkfloat(l, NAME (isfloat(car(args)) ? floatval(car(args)) :\
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
        if(!cklen(args, 2) || !isarith(car(args)) || !isarith(car(cdr(args))))
                RECOVER(l, "\"expected (number number)\" '%S", args);
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
                RECOVER(l, "\"expected (number)\" '%S", args);
        xo = car(args);
        x = isfloat(xo) ? floatval(xo) : intval(xo);
        fracpart = modf(x, &intpart);
        return cons(l, mkfloat(l, intpart), mkfloat(l, fracpart));
}

#ifdef USE_LINE
/*line editing and history functionality*/
static char *line_editing_function(const char *prompt) {
        static int warned = 0; /**< have we warned the user we cannot write to
                                    the history file?*/
        char *line;
        running = 0; /*SIGINT handling off when reading input*/
        line = line_editor(prompt);
        /*do not add blank lines*/
        if(!line || !line[strspn(line, " \t\r\n")]) return line;
        line_history_add(line);
        if(line_history_save(histfile) && !warned) {
                PRINT_ERROR("\"could not save history\" \"%s\"", histfile);
                warned = 1;
        }
        assert(lglobal); /*lglobal needed for signal handler*/
        if(signal(SIGINT, sig_int_handler) == SIG_ERR) /*(re)install handler*/
                PRINT_ERROR("\"%s\"", "could not set signal handler");
        running = 1; /*SIGINT handling on when evaluating input*/
        return line; /*returned line will be evaluated*/
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
        if(!cklen(args, 1) || !isint(car(args)))
                RECOVER(l, "\"expected (integer)\" '%S", args);
        if(!line_history_set_maxlen((int)intval(car(args))))
                HALT(l, "\"%s\"", "out of memory");
        return (cell*)mktee();
}

static cell *subr_clear_screen(lisp *l, cell *args) {
        (void)args;
        if(!cklen(args, 0))
                RECOVER(l, "\"expected ()\" '%S", args);
        line_clearscreen();
        return (cell*)mktee();
}
#endif

int main(int argc, char **argv) {
        int r;
        lisp *l = lisp_init();
        if(!l) return PRINT_ERROR("\"%s\"", "initialization failed"), -1;

        if(signal(SIGINT, sig_int_handler) == SIG_ERR)
                PRINT_ERROR("\"%s\"", "could not set signal handler");
        lglobal = l;

        lisp_add_cell(l, "*version*",           mkstr(l, lstrdup(XSTRINGIFY(VERSION))));
        lisp_add_cell(l, "*commit*",            mkstr(l, lstrdup(XSTRINGIFY(VCS_COMMIT))));
        lisp_add_cell(l, "*repository-origin*", mkstr(l, lstrdup(XSTRINGIFY(VCS_ORIGIN))));

#define X(FUNC) lisp_add_subr(l, # FUNC, subr_ ## FUNC);
MATH_UNARY_LIST
#undef X
        lisp_add_subr(l, "pow", subr_pow);
        lisp_add_subr(l, "modf", subr_modf);
        lisp_add_cell(l, "*have-math*", (cell*)mktee());

#ifdef USE_LINE /*add line editor functionality*/
        static char *homedir;
        homedir = getenv("HOME"); /*Unix home path*/
        if(homedir) /*if found put the history file there*/
                histfile = VSTRCATSEP("/", homedir, histfile);
        if(!histfile)
                PRINT_ERROR("\"%s\"", "VSTRCATSEP allocation failed");
        lisp_set_line_editor(l, line_editing_function);
        line_history_load(histfile);
        line_set_vi_mode(1); /*start up in a sane editing mode*/
        lisp_add_subr(l, "line-editor-mode", subr_line_editor_mode);
        lisp_add_subr(l, "clear-screen",     subr_clear_screen);
        lisp_add_subr(l, "history-length",   subr_hist_len);
        lisp_add_cell(l, "*history-file*",   mkstr(l, lstrdup(histfile)));
        lisp_add_cell(l, "*have-line*",      (cell*)mktee());
#else
        lisp_add_cell(l, "*have-line*", (cell*)mknil());
#endif

        r = main_lisp_env(l, argc, argv);
#ifdef USE_LINE
        if(homedir) free(histfile);
#endif
        return r;
}

