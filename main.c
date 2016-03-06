/** @file       main.c
 *  @brief      A minimal lisp interpreter and utility library, simple driver
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *
 *  All of the non-portable code in the interpreter is isolated here, the
 *  library itself is written in pure C (C99) and dependent only on the
 *  functions within the standard C library. This file adds in support
 *  for various things depending on the operating system (if known). The
 *  only use of horrible ifdefs to select code should be in this file (and
 *  any modules which need to be portable across Unix and Windows).
 **/

#include "liblisp.h"
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
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

#ifdef __unix__
#include <dlfcn.h>
static char *os   = "unix";
#elif _WIN32
#include <windows.h>
static char *os   = "windows";
#else
static char *os   = "unknown";
#endif

#ifdef USE_INITRC
static char *init = ".lisprc"; /**< lisp initialization file */
#ifdef __unix__
static char *home = "HOME"; /**< Unix home path*/
static char *filesep = "/"; /**< Unix file directory separator */
#elif _WIN32
static char *home = "HOMEPATH"; /**< Windows home path*/
static char *filesep = "\\"; /**< Window file directory separator */
#else
static char *home = ""; /**< unknown operating system*/
static char *filesep = "/"; /**< unknown os file directory separator */
#endif
#else
static char *home = ""; /**< unknown operating system*/
static char *filesep = "/"; /**< unknown os file directory separator */
#endif

#ifdef USE_ABORT_HANDLER
#ifdef __unix__
/* it should be possible to move this into a module that can be loaded,
 * however it would only be able to catch SIGABRT after the interpreter
 * is in a working state already, making it less useful */
#include <execinfo.h>
#define TRACE_SIZE      (64u)

/** @warning this hander calls functions that are not safe to call
 *           from a signal handler, however this is only going to
 *           be called in the event of an internal consistency failure,
 *           and only as a courtesy to the programmer
 *  @todo make a windows version using information from:
 *  https://msdn.microsoft.com/en-us/library/windows/desktop/bb204633%28v=vs.85%29.aspx and
 *  https://stackoverflow.com/questions/5693192/win32-backtrace-from-c-code
 *  @todo add a function that prints stack traces in the lisp environment
 *  */
static void sig_abrt_handler(int sig) {
        void *trace[TRACE_SIZE];
        char **messages = NULL;
        int i, trace_size;
        trace_size = backtrace(trace, TRACE_SIZE);
        messages = backtrace_symbols(trace, trace_size);
        if(trace_size < 0)
                goto fail;
        fprintf(stderr, "SIGABRT! Stack trace:\n");
        for(i = 0; i < trace_size; i++)
                fprintf(stderr, "\t%s\n", messages[i]);
        fflush(stderr);
fail:   signal(sig, SIG_DFL);
        abort();
}
#endif
#endif

/*  @todo The module interface should made so multiple threads running lisp
 *  interpreter can load the same module. This can be done by the DL_OPEN
 *  routine looking for a constructor function that must be included in each
 *  module, DL_SYM would look for it. It must avoid memory allocation in
 *  the modules DllMain (on Windows only) due to problems mentioned here:
 *  https://stackoverflow.com/questions/858592/windows-malloc-replacement-e-g-tcmalloc-and-dynamic-crt-linking?rq=1
 *  and
 *  https://msdn.microsoft.com/en-us/library/windows/desktop/dn633971%28v=vs.85%29.aspx#general_best_practices
 **/
#ifdef USE_DL
/* Module loader using dlopen/LoadLibrary, all functions acquired with 
 * dlsym/GetProcAddress must be of the "subr" function type as they will 
 * be used as internal lisp subroutines by the interpreter. */

#ifdef __unix__ /*Only tested on Linux*/
typedef void* dl_handle_t;

#define DL_OPEN(NAME)        dlopen((NAME), RTLD_NOW)
#define DL_CLOSE(HANDLE)     dlclose((HANDLE))
#define DL_SYM(HANDLE, NAME) dlsym((HANDLE), (NAME))
#define DL_ERROR()	     dlerror()

#elif _WIN32 /*Windows*/
typedef HMODULE dl_handle_t;

#define DL_OPEN(NAME)        LoadLibrary((NAME))
#define DL_CLOSE(HANDLE)     FreeLibrary((HANDLE))
#define DL_SYM(HANDLE, NAME) (subr)GetProcAddress((HMODULE)(HANDLE), (NAME))
#define DL_ERROR()           win_dlerror()

static char *win_dlerror(void) {
        static char buf[256] = "";
        FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, GetLastError(), 
              MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), buf, 256, NULL);
        return buf;
}

#else
#error "Unrecognized platform"
#endif

static int ud_dl = 0; /**< User defined type value for DLL handles*/

/**@bug This should be locked when is use!*/
struct dl_list; /**< linked list of all DLL handles*/
typedef struct dl_list {
        dl_handle_t handle;
        struct dl_list *next; /**< next node in linked list*/
} dl_list;

dl_list *head; /**< *GLOBAL* list of all DLL handles for dlclose_atexit*/

/** @brief close all of the open DLLs when the program exits, subr_dlopen
 *         adds the handles to this list **/
static void dlclose_atexit(void) {
        dl_list *t; 
        while(head) {
                assert(head->handle);
                DL_CLOSE(head->handle); /*closes DLL and calls its destructors*/
                t = head;
                head = head->next;
                free(t);
        }
}

static void ud_dl_free(cell *f) {
      /*DL_CLOSE(get_user(f)); This is handled atexit instead*/
        free(f);
}

static int ud_dl_print(io *o, unsigned depth, cell *f) {
        return lisp_printf(NULL, o, depth, "%B<DYNAMIC-MODULE:%d>%t", get_user(f));
}

static cell *subr_dlopen(lisp *l, cell *args) {
	dl_handle_t handle;
        dl_list *h;
        if(!(handle = DL_OPEN(get_str(car(args))))) {
		lisp_log_debug(l, "'dynamic-load-failed \"%s\" \"%s\"", get_str(car(args)), DL_ERROR());
                return gsym_error();
	}
        if(!(h = calloc(1, sizeof(*h))))
                LISP_HALT(l, "\"%s\"", "out of memory");
        h->handle = handle;
        h->next = head;
        head = h;
        return mk_user(l, handle, ud_dl);
}

/* loads a lisp module and runs the initialization function */
static cell *subr_load_lisp_module(lisp *l, cell *args) {
	cell *h = subr_dlopen(l, args);
	dl_handle_t handle;
	lisp_module_initializer_t init;
	if(!is_usertype(h, ud_dl))
		return gsym_error();
	handle = get_user(h);
	lisp_log_debug(l, "'module-initialization \"%s\"", get_str(car(args)));
	if((init = DL_SYM(handle, "lisp_module_initialize")) && (init(l) >= 0)) {
		lisp_log_note(l, "'module-initialized \"%s\"", get_str(car(args)));
		return h;
	}
	lisp_log_error(l, "'module-initialization \"%s\"", get_str(car(args)));
	return gsym_error();
}

static cell *subr_dlsym(lisp *l, cell *args) {
        subr func;
        if(!cklen(args, 2) || !is_usertype(car(args), ud_dl) || !is_asciiz(CADR(args)))
                LISP_RECOVER(l, "\"expected (dynamic-module string)\" '%S", args);
        if(!(func = DL_SYM(get_user(car(args)), get_str(CADR(args)))))
                return gsym_error();
        return mk_subr(l, func, NULL, NULL);
}

static cell *subr_dlerror(lisp *l, cell *args) {
        char *s = DL_ERROR();
	UNUSED(args);
        return mk_str(l, lisp_strdup(l, (s = DL_ERROR()) ? s : ""));
}
#endif

int main(int argc, char **argv) {
        lisp *l = lisp_init();
        if(!l) 
                goto fail;

        lisp_add_cell(l, "*version*",           mk_str(l, lstrdup_or_abort(XSTRINGIFY(VERSION))));
        lisp_add_cell(l, "*commit*",            mk_str(l, lstrdup_or_abort(XSTRINGIFY(VCS_COMMIT))));
        lisp_add_cell(l, "*repository-origin*", mk_str(l, lstrdup_or_abort(XSTRINGIFY(VCS_ORIGIN))));
	lisp_add_cell(l, "*os*",                mk_str(l, lstrdup_or_abort(os)));

#ifdef USE_DL
        ud_dl = new_user_defined_type(l, ud_dl_free, NULL, NULL, ud_dl_print);
        if(ud_dl < 0)
                goto fail;
        lisp_add_subr(l, "dynamic-open",   subr_dlopen, "Z", NULL);
        lisp_add_subr(l, "dynamic-symbol", subr_dlsym, NULL, NULL);
        lisp_add_subr(l, "dynamic-error",  subr_dlerror, "", NULL);
        lisp_add_subr(l, "dynamic-load-lisp-module", subr_load_lisp_module, "Z", NULL);
        lisp_add_cell(l, "*have-dynamic-loader*", gsym_tee());
        atexit(dlclose_atexit);
#else
        lisp_add_cell(l, "*have-dynamic-loader*", gsym_nil());
#endif

#ifdef USE_ABORT_HANDLER
#ifdef __unix__
        if(signal(SIGABRT, sig_abrt_handler) == SIG_ERR) {
                PRINT_ERROR("\"%s\"", "could not install SIGABRT handler");
                goto fail;
        }
#endif
#endif

        lisp_add_cell(l, "*file-separator*", mk_str(l, lstrdup_or_abort(filesep)));
#ifdef USE_INITRC
        char *rcpath = NULL, *thome = NULL;
	/*LISPHOME takes precedence over HOME*/
	if((thome = getenv("LISPHOME")))
		rcpath = thome;
        if(rcpath || (rcpath = getenv(home))) {
                io *i;
                FILE *in;
                if(!(rcpath = VSTRCATSEP(filesep, rcpath, init)))
                        goto fail;
                if((in = fopen(rcpath, "rb"))) {
                        i = lisp_get_input(l);
                        io_close(i);

                        if(!(i = io_fin(in)))
                                goto fail;
                        lisp_set_input(l, i);
                        if(lisp_repl(l, "", 0) < 0) {
                                PRINT_ERROR("\"error in initialization file\" \"%s\"", rcpath);
                                goto fail;
                        }
                        io_close(i);
                        if(!(i = io_fin(stdin)))
                                goto fail;
                        lisp_set_input(l, i);
                }
                lisp_add_cell(l, "*initialization-file*", mk_str(l, rcpath));
        }
#endif
        return main_lisp_env(l, argc, argv);
fail:   
	PRINT_ERROR("\"%s\"", "initialization failed");
        return -1;
}

