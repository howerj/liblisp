/** @file       main.c
 *  @brief      A minimal lisp interpreter and utility library, simple driver
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com**/

#include "liblisp.h"
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

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

/**@brief Template for most of the functions in "math.h"
 * @param NAME name of math function such as "log", "sin", etc.*/
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

#define SUBR_ISX(NAME)\
static cell *subr_ ## NAME (lisp *l, cell *args) {\
        char *s, c;\
        if(cklen(args, 1) && isint(car(args)))\
                return NAME (intval(car(args))) ? mktee() : (cell*)mknil();\
        if(!cklen(args, 1) || !isasciiz(car(args)))\
                RECOVER(l, "\"expected (string)\" %S", args);\
        s = strval(car(args));\
        if(!s[0]) return mknil();\
        while((c = *s++)) \
                if(! NAME (c))\
                        return mknil();\
        return mktee();\
}

#define ISX_LIST\
        X(isalnum) X(isalpha) X(iscntrl)\
        X(isdigit) X(isgraph) X(islower)\
        X(isprint) X(ispunct) X(isspace)\
        X(isupper) X(isxdigit)

#define X(FUNC) SUBR_ISX(FUNC)
ISX_LIST
#undef X

static cell *subr_pow (lisp *l, cell *args) {
        cell *xo, *yo;
        double x, y;
        if(!cklen(args, 2) || !isarith(car(args)) || !isarith(CADR(args)))
                RECOVER(l, "\"expected (number number)\" '%S", args);
        xo = car(args);
        yo = CADR(args);
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
#include "libline/libline.h"
static char *histfile = ".list";

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
                return mktee();
        }
        return line_get_vi_mode() ? mktee(): (cell*)mknil();
}

static cell *subr_hist_len(lisp *l, cell *args) {
        if(!cklen(args, 1) || !isint(car(args)))
                RECOVER(l, "\"expected (integer)\" '%S", args);
        if(!line_history_set_maxlen((int)intval(car(args))))
                HALT(l, "\"%s\"", "out of memory");
        return mktee();
}

static cell *subr_clear_screen(lisp *l, cell *args) {
        (void)args;
        if(!cklen(args, 0))
                RECOVER(l, "\"expected ()\" '%S", args);
        line_clearscreen();
        return mktee();
}
#endif

#ifdef USE_TCC
#include <libtcc.h>
static int ud_tcc = 0;

static void ud_tcc_free(cell *f) {
        tcc_delete(userval(f));
        free(f);
}

static int ud_tcc_print(io *o, unsigned depth, cell *f) {
        return printerf(NULL, o, depth, "%B<COMPILE-STATE:%d>%t", userval(f));
}

static cell* subr_compile(lisp *l, cell *args) {
        if(!cklen(args, 3) 
        || !isusertype(car(args), ud_tcc)
        || !isasciiz(CADR(args)) || !isstr(CADDR(args)))
                RECOVER(l, "\"expected (compile-state string string\" '%S", args);
        char *fname = strval(CADR(args)), *prog = strval(CADDR(args));
        subr func;
        TCCState *st = userval(car(args));
        if(tcc_compile_string(st, prog) < 0)
                return mkerror();
        if(tcc_relocate(st, TCC_RELOCATE_AUTO) < 0)
                return mkerror();
        func = (subr)tcc_get_symbol(st, fname);
        return mksubr(l, func);
}

static cell* subr_link(lisp *l, cell *args) {
        if(!cklen(args, 2) 
        || !isusertype(car(args), ud_tcc) || !isasciiz(CADR(args)))
                RECOVER(l, "\"expected (compile-state string)\" '%S", args);
        return mkint(l, tcc_add_library(userval(car(args)), strval(CADR(args))));
}

static cell* subr_compile_file(lisp *l, cell *args) {
        if(!cklen(args, 2)
        || !isusertype(car(args), ud_tcc) || !isasciiz(CADR(args)))
                RECOVER(l, "\"expected (compile-state string)\" '%S", args);
        if(tcc_add_file(userval(car(args)), strval(CADR(args))) < 0)
                return mkerror();
        if(tcc_relocate(userval(car(args)), TCC_RELOCATE_AUTO) < 0)
                return mkerror();
        return mktee();
}

static cell* subr_get_subr(lisp *l, cell *args) {
        subr func;
        if(!cklen(args, 2)
        || !isusertype(car(args), ud_tcc) || !isasciiz(CADR(args)))
                RECOVER(l, "\"expected (compile-state string)\" '%S", args);
        if(!(func = tcc_get_symbol(userval(car(args)), strval(CADR(args)))))
                return mkerror();
        else
                return mksubr(l, func);
}
#endif

#ifdef USE_DL
#include <dlfcn.h>
static int ud_dl = 0;

static void ud_dl_free(cell *f) {
        dlclose(userval(f));
        free(f);
}

static int ud_dl_print(io *o, unsigned depth, cell *f) {
        return printerf(NULL, o, depth, "%B<DYNAMIC-MODULE:%d>%t", userval(f));
}

static cell *subr_dlopen(lisp *l, cell *args) {
        void *handle;
        if(!cklen(args, 1) || !isasciiz(car(args)))
                RECOVER(l, "\"expected (string)\" '%S", args);
        if(!(handle = dlopen(strval(car(args)), RTLD_NOW)))
                return mkstr(l, lstrdup(dlerror()));
        return mkuser(l, handle, ud_dl);
}

static cell *subr_dlsym(lisp *l, cell *args) {
        subr func;
        if(!cklen(args, 2) || !isusertype(car(args), ud_dl) || !isasciiz(CADR(args)))
                RECOVER(l, "\"expected (dynamic-module string)\" '%S", args);
        if(!(func = dlsym(userval(car(args)), strval(CADR(args)))))
                return mkerror();
        return mksubr(l, func);
}

static cell *subr_dlerror(lisp *l, cell *args) {
        if(!cklen(args, 1) || !isusertype(car(args), ud_dl))
                RECOVER(l, "\"expected (dynamic-module)\" '%S", args);
        return mkerror();
}
#endif

int main(int argc, char **argv) {
        int r;
        lisp *l = lisp_init();
        if(!l) return PRINT_ERROR("\"%s\"", "initialization failed"), -1;

        if(signal(SIGINT, sig_int_handler) == SIG_ERR)
                PRINT_ERROR("\"%s\"", "could not set signal handler");
        lglobal = l;

#define X(FUNC) lisp_add_subr(l, # FUNC, subr_ ## FUNC);
MATH_UNARY_LIST
#undef X
        lisp_add_subr(l, "pow",  subr_pow);
        lisp_add_subr(l, "modf", subr_modf);
        lisp_add_cell(l, "*have-math*", mktee());

#define X(FUNC) lisp_add_subr(l, # FUNC "?", subr_ ## FUNC);
ISX_LIST
#undef X
        lisp_add_cell(l, "*version*",           mkstr(l, lstrdup(XSTRINGIFY(VERSION))));
        lisp_add_cell(l, "*commit*",            mkstr(l, lstrdup(XSTRINGIFY(VCS_COMMIT))));
        lisp_add_cell(l, "*repository-origin*", mkstr(l, lstrdup(XSTRINGIFY(VCS_ORIGIN))));

#ifdef USE_TCC
        /** Tiny C compiler library interface, special care has to be taken 
         *  when compiling and linking all of the C files within the liblisp
         *  project so the symbols in it are available to libtcc.
         *
         * This example creates a function that returns the integer "3" from
         * within the interpreter:
         *
         * > (define three-func 
         *      (compile *compile-state* "p" 
         *        "void *p(void *l, void *a) { return (void*)mkint(l, 3); }"))
         * <SUBR:35097280>
         * > (three-func)
         * 3
         *
         ***/
        ud_tcc = newuserdef(l, ud_tcc_free, NULL, NULL, ud_tcc_print);
        TCCState *st = tcc_new();
        tcc_set_output_type(st, TCC_OUTPUT_MEMORY);
        lisp_add_cell(l, "*compile-state*", mkuser(l, st, ud_tcc));
        lisp_add_subr(l, "compile",        subr_compile);
        lisp_add_subr(l, "link",           subr_link);
        lisp_add_subr(l, "compile-file",   subr_compile_file);
        lisp_add_subr(l, "get-subroutine", subr_get_subr);
        lisp_add_cell(l, "*have-compile*", mktee());
#else
        lisp_add_cell(l, "*have-compile*", mknil());
#endif

#ifdef USE_DL
        ud_dl = newuserdef(l, ud_dl_free, NULL, NULL, ud_dl_print);
        lisp_add_subr(l, "dynamic-open", subr_dlopen);
        lisp_add_subr(l, "dynamic-symbol", subr_dlsym);
        lisp_add_subr(l, "dynamic-error", subr_dlerror);
#endif

#ifdef USE_LINE 
        /* This adds line editor functionality from the "libline" library,
         * this is a fork of the "linenoise" library.*/
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
        lisp_add_cell(l, "*have-line*",      mktee());
#else
        lisp_add_cell(l, "*have-line*",      mknil());
#endif
        r = main_lisp_env(l, argc, argv);
#ifdef USE_LINE
        if(homedir) free(histfile);
#endif
        return r;
}

