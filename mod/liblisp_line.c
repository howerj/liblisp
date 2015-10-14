/** @file       liblisp_line.c
 *  @brief      Line editor module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com**/
#include <assert.h>
#include <libline.h>
#include <liblisp.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static char *homedir;
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

#define SUBROUTINE_XLIST\
        X("line-editor-mode", subr_line_editor_mode, "b", "set the line editor mode (t = vi-mode)")\
        X("clear-screen",     subr_clear_screen,     "",  "clear the screen")\
        X("history-length",   subr_hist_len,         "d", "set the length of the history file")

static char *histfile = ".lisp";

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

static cell *subr_line_editor_mode(lisp *l, cell *args) { UNUSED(l);
        line_set_vi_mode(is_nil(car(args)) ? 0 : 1);
        return gsym_tee();
}

static cell *subr_hist_len(lisp *l, cell *args) {
        if(!line_history_set_maxlen((int)get_int(car(args))))
                HALT(l, "\"%s\"", "out of memory");
        return gsym_tee();
}

static cell *subr_clear_screen(lisp *l, cell *args) { UNUSED(l); UNUSED(args);
        line_clearscreen();
        return gsym_tee();
}

#define X(NAME, SUBR, VALIDATION, DOCSTRING) static cell* SUBR (lisp *l, cell *args);
SUBROUTINE_XLIST /*function prototypes for all of the built-in subroutines*/
#undef X

#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, DOCSTRING, SUBR },
static struct module_subroutines { char *name, *validate, *docstring; subr p; } primitives[] = {
        SUBROUTINE_XLIST /*all of the subr functions*/
        {NULL, NULL, NULL, NULL} /*must be terminated with NULLs*/
};
#undef X

static int initialize(void) {
        size_t i;
        io *e;
        assert(lglobal);
        e = lisp_get_logging(lglobal);

        if(signal(SIGINT, sig_int_handler) == SIG_ERR) {
                PRINT_ERROR("\"%s\"", "could not set signal handler");
                goto fail;
        }
        /* This adds line editor functionality from the "libline" library,
         * this is a fork of the "linenoise" library.*/
        homedir = getenv("HOME"); /*Unix home path*/
        if(homedir) /*if found put the history file there*/
                histfile = VSTRCATSEP("/", homedir, histfile);
        if(!histfile)
                PRINT_ERROR("\"%s\"", "VSTRCATSEP allocation failed");
        lisp_set_line_editor(lglobal, line_editing_function);
        line_history_load(histfile);
        line_set_vi_mode(1); /*start up in a sane editing mode*/
        lisp_add_cell(lglobal, "*history-file*", mk_str(lglobal, lstrdup(histfile)));

        for(i = 0; primitives[i].p; i++) /*add primitives from this module*/
               if(!lisp_add_subr(lglobal, 
                        primitives[i].name, primitives[i].p, 
                        primitives[i].validate, primitives[i].docstring))
                     goto fail;
        lisp_printf(lglobal, e, 0, "module: line editor loaded\n");
        return 0;
fail:   lisp_printf(lglobal, e, 0, "module: line editor load failure\n");
        return -1;
}

#ifdef __unix__
static void construct(void) __attribute__((constructor));
static void destruct(void)  __attribute__((destructor));
static void construct(void) { initialize(); }
static void destruct(void)  { 
       /*lisp_set_line_editor(lglobal, NULL);*/ 
         if(homedir) free(histfile); 
}
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

