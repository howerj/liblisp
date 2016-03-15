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
#include <lispmod.h>
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <locale.h>

#ifdef __unix__
static char *os   = "unix";
#elif _WIN32
static char *os   = "windows";
#else
static char *os   = "unknown";
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
static void sig_abrt_handler(int sig) 
{
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

#ifdef USE_MUTEX
#ifdef __unix__
/*Supported*/
#elif  _WIN32
/*Supported*/
#else
#error "USE_MUTEX not supported on Unknown platform"
#endif
/**@todo improve this code and test it on Windows, also add it to a liblisp
 * module header.
 *
 * See: https://stackoverflow.com/questions/3555859/is-it-possible-to-do-static-initialization-of-mutexes-in-windows*/

lisp_mutex_t* lisp_mutex_create(void)
{
        lisp_mutex_t* p;
#ifdef __unix__
        if(!(p = calloc(1, sizeof(pthread_mutex_t))))
                return NULL;
        pthread_mutex_init(p, NULL);
        return p;
#elif _WIN32 
        if(!(p = calloc(1, sizeof(CRITICAL_SECTION))))
                return NULL;
        InitializeCriticalSection(p);
        return p; 
#endif
}

int lisp_mutex_lock(lisp_mutex_t *m) 
{
#ifdef __unix__
        return pthread_mutex_lock(m); 
#elif  _WIN32
        EnterCriticalSection((LPCRITICAL_SECTION)m);
        return 0; 
#endif
}

int lisp_mutex_trylock(lisp_mutex_t *m) 
{
#ifdef __unix__
        return pthread_mutex_trylock(m); 
#elif  _WIN32
	return TryEnterCriticalSection(m);
#endif
}

int lisp_mutex_unlock(lisp_mutex_t *m) 
{ 
#ifdef __unix__
        return pthread_mutex_unlock(m); 
#elif  _WIN32
        LeaveCriticalSection((LPCRITICAL_SECTION)m);
        return 0;
#endif
}

#endif

#ifdef USE_DL
/* Module loader using dlopen/LoadLibrary, all functions acquired with 
 * dlsym/GetProcAddress must be of the "subr" function type as they will 
 * be used as internal lisp subroutines by the interpreter. */

#ifdef __unix__ /*Only tested on Linux, not other Unixen */
/*Supported*/

const char *lisp_mod_dlerror(void) 
{
	return dlerror();
}

#elif _WIN32 /*Windows*/
/*Supported*/

const char *lisp_mod_dlerror(void) 
{
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
static void dlclose_atexit(void) 
{
        dl_list *t; 
        while(head) {
                assert(head->handle);
                DL_CLOSE(head->handle); /*closes DLL and calls its destructors*/
                t = head;
                head = head->next;
                free(t);
        }
}

static void ud_dl_free(lisp_cell_t *f) 
{
      /*DL_CLOSE(get_user(f)); This is handled atexit instead*/
        free(f);
}

static int ud_dl_print(io_t *o, unsigned depth, lisp_cell_t *f) 
{
        return lisp_printf(NULL, o, depth, "%B<DYNAMIC-MODULE:%d>%t", get_user(f));
}

static lisp_cell_t *subr_dlopen(lisp_t *l, lisp_cell_t *args) 
{
	dl_handle_t handle;
        dl_list *h;
        if(!(handle = DL_OPEN(get_str(car(args))))) {
		lisp_log_error(l, "'dynamic-load-failed \"%s\" \"%s\"", get_str(car(args)), DL_ERROR());
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
static lisp_cell_t *subr_load_lisp_module(lisp_t *l, lisp_cell_t *args) 
{
	lisp_cell_t *h = subr_dlopen(l, args);
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

static lisp_cell_t *subr_dlsym(lisp_t *l, lisp_cell_t *args) 
{
        lisp_subr_func func;
        if(!lisp_check_length(args, 2) || !is_usertype(car(args), ud_dl) || !is_asciiz(CADR(args)))
                LISP_RECOVER(l, "\"expected (dynamic-module string)\" '%S", args);
        if(!(func = DL_SYM(get_user(car(args)), get_str(CADR(args)))))
                return gsym_error();
        return mk_subr(l, func, NULL, NULL);
}

static lisp_cell_t *subr_dlerror(lisp_t *l, lisp_cell_t *args) 
{
        const char *s = DL_ERROR();
	UNUSED(args);
        return mk_str(l, lisp_strdup(l, (s = DL_ERROR()) ? s : ""));
}
#endif

int main(int argc, char **argv) 
{
        lisp_t *l;

	ASSERT(l = lisp_init());

	lisp_add_cell(l, "*os*", mk_str(l, lstrdup_or_abort(os)));
#ifdef USE_DL
        ASSERT((ud_dl = new_user_defined_type(l, ud_dl_free, NULL, NULL, ud_dl_print)) >= 0);

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
	ASSERT(signal(SIGABRT, sig_abrt_handler) != SIG_ERR);
#endif
#endif
        return main_lisp_env(l, argc, argv);
}

