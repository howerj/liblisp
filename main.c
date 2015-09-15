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

lisp *lglobal; /**< used for signal handler and modules*/

#ifdef USE_DL
/* Module loader using dlopen, all functions acquired with dlsym must be of the
 * "subr" function type as they will be used as internal lisp subroutines by
 * the interpreter. */
#include <dlfcn.h>
static int ud_dl = 0;

struct dllist;
typedef struct dllist {
        void* handle;
        struct dllist* next;
} dllist;

dllist *head;

static void dlclose_atexit(void) {
        dllist *t;
        if(!head) return;
        do {
                dlclose(head->handle);
                t = head;
                head = head->next;
                free(t);
        } while(head);
}

static void ud_dl_free(cell *f) {
        /** @bug There is a problem with closing modules that contain 
         *       callbacks like these for freeing user defined types 
         *       made within those modules. If dlclose is called before
         *       before the callback in the closed module is called the
         *       program will SEGFAULT as the callback no longer is mapped
         *       into the program space. **/
      /*dlclose(userval(f));*/
        free(f);
}

static int ud_dl_print(io *o, unsigned depth, cell *f) {
        return printerf(NULL, o, depth, "%B<DYNAMIC-MODULE:%d>%t", userval(f));
}

static cell *subr_dlopen(lisp *l, cell *args) {
        void *handle;
        dllist *h;
        if(!cklen(args, 1) || !is_asciiz(car(args)))
                RECOVER(l, "\"expected (string)\" '%S", args);
        if(!(handle = dlopen(strval(car(args)), RTLD_NOW)))
                return gsym_error();
        if(!(h = calloc(1, sizeof(*h))))
                HALT(l, "\"%s\"", "out of memory");
        h->handle = handle;
        h->next = head;
        head = h;
        return mkuser(l, handle, ud_dl);
}

static cell *subr_dlsym(lisp *l, cell *args) {
        subr func;
        if(!cklen(args, 2) || !is_usertype(car(args), ud_dl) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (dynamic-module string)\" '%S", args);
        if(!(func = dlsym(userval(car(args)), strval(CADR(args)))))
                return gsym_error();
        return mksubr(l, func);
}

static cell *subr_dlerror(lisp *l, cell *args) {
        char *s;
        if(!cklen(args, 0))
                RECOVER(l, "\"expected ()\" '%S", args);
        return mkstr(l, lstrdup((s = dlerror()) ? s : ""));
}
#endif

int main(int argc, char **argv) {
        int r;
        lisp *l = lisp_init();
        if(!l) return PRINT_ERROR("\"%s\"", "initialization failed"), -1;

        lglobal = l;

        lisp_add_cell(l, "*version*",           mkstr(l, lstrdup(XSTRINGIFY(VERSION))));
        lisp_add_cell(l, "*commit*",            mkstr(l, lstrdup(XSTRINGIFY(VCS_COMMIT))));
        lisp_add_cell(l, "*repository-origin*", mkstr(l, lstrdup(XSTRINGIFY(VCS_ORIGIN))));

#ifdef USE_DL
        ud_dl = newuserdef(l, ud_dl_free, NULL, NULL, ud_dl_print);
        lisp_add_subr(l, "dynamic-open",   subr_dlopen);
        lisp_add_subr(l, "dynamic-symbol", subr_dlsym);
        lisp_add_subr(l, "dynamic-error",  subr_dlerror);
        lisp_add_cell(l, "*have-dynamic-loader*", gsym_tee());
        atexit(dlclose_atexit);
#else
        lisp_add_cell(l, "*have-dynamic-loader*", gsym_nil());
#endif

        r = main_lisp_env(l, argc, argv);
        return r;
}

