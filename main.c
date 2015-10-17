/** @file       main.c
 *  @brief      A minimal lisp interpreter and utility library, simple driver
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com**/

#include "liblisp.h"
#include <assert.h>
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

#ifdef __unix__
static char *os = "unix";
#elif _WIN32
#include <windows.h>
static char *os = "windows";
#else
static char *os = "unknown";
#endif

#ifdef USE_DL
/* Module loader using dlopen/LoadLibrary, all functions acquired with 
 * dlsym/GetProcAddress must be of the "subr" function type as they will 
 * be used as internal lisp subroutines by the interpreter. */

#ifdef __unix__

#include <dlfcn.h>
typedef void* handle_t;

#define DL_OPEN(NAME)        dlopen((NAME), RTLD_NOW)
#define DL_CLOSE(HANDLE)     dlclose((HANDLE))
#define DL_SYM(HANDLE, NAME) dlsym((HANDLE), (NAME))
#define DL_ERROR()	     dlerror()

#elif _WIN32

typedef HMODULE handle_t;

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

struct dllist; /**< linked list of all DLL handles*/
typedef struct dllist {
        handle_t handle;
        struct dllist *next; /**< next node in linked list*/
} dllist;

dllist *head; /**< *GLOBAL* list of all DLL handles for dlclose_atexit*/

/** @brief close all of the open DLLs when the program exits **/
static void dlclose_atexit(void) {
        dllist *t; 
        while(head) {
                assert(head->handle);
                DL_CLOSE(head->handle);
                t = head;
                head = head->next;
                free(t);
        }
}

static void ud_dl_free(cell *f) {
      /* There is a problem with closing modules that contain 
       * callbacks like these for freeing user defined types 
       * made within those modules. If dlclose is called before
       * before the callback in the closed module is called the
       * program will SEGFAULT as the callback no longer is mapped
       * into the program space. This is resolved with dlclose_atexit */
      /*DL_CLOSE(get_user(f));*/
        free(f);
}

static int ud_dl_print(io *o, unsigned depth, cell *f) {
        return lisp_printf(NULL, o, depth, "%B<DYNAMIC-MODULE:%d>%t", get_user(f));
}

static cell *subr_dlopen(lisp *l, cell *args) {
	handle_t handle;
        dllist *h;
        if(!(handle = DL_OPEN(get_str(car(args)))))
                return gsym_error();
        if(!(h = calloc(1, sizeof(*h))))
                HALT(l, "\"%s\"", "out of memory");
        h->handle = handle;
        h->next = head;
        head = h;
        return mk_user(l, handle, ud_dl);
}

static cell *subr_dlsym(lisp *l, cell *args) {
        subr func;
        if(!cklen(args, 2) || !is_usertype(car(args), ud_dl) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (dynamic-module string)\" '%S", args);
        if(!(func = DL_SYM(get_user(car(args)), get_str(CADR(args)))))
                return gsym_error();
        return mk_subr(l, func, NULL, NULL);
}

static cell *subr_dlerror(lisp *l, cell *args) {
        char *s;
        if(!cklen(args, 0))
                RECOVER(l, "\"expected ()\" '%S", args);
        return mk_str(l, lstrdup((s = DL_ERROR()) ? s : ""));
}
#endif

int main(int argc, char **argv) {
        lisp *l = lisp_init();
        if(!l) 
                goto fail;
        lglobal = l;

        lisp_add_cell(l, "*version*",           mk_str(l, lstrdup(XSTRINGIFY(VERSION))));
        lisp_add_cell(l, "*commit*",            mk_str(l, lstrdup(XSTRINGIFY(VCS_COMMIT))));
        lisp_add_cell(l, "*repository-origin*", mk_str(l, lstrdup(XSTRINGIFY(VCS_ORIGIN))));
	lisp_add_cell(l, "*os*",                mk_str(l, lstrdup(os)));

#ifdef USE_DL
        ud_dl = new_user_defined_type(l, ud_dl_free, NULL, NULL, ud_dl_print);
        if(ud_dl < 0)
                goto fail;
        lisp_add_subr(l, "dynamic-open",   subr_dlopen, "Z", NULL);
        lisp_add_subr(l, "dynamic-symbol", subr_dlsym, NULL, NULL);
        lisp_add_subr(l, "dynamic-error",  subr_dlerror, "", NULL);
        lisp_add_cell(l, "*have-dynamic-loader*", gsym_tee());
        atexit(dlclose_atexit);
#else
        lisp_add_cell(l, "*have-dynamic-loader*", gsym_nil());
#endif

        return main_lisp_env(l, argc, argv);
fail:   return PRINT_ERROR("\"%s\"", "initialization failed"), -1;
        return -1;
}

