/** @file       liblisp_bignum.c
 *  @brief      arbitrary precision arithmetic module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com **/
#include <assert.h>
#include <liblisp.h>
#include <stdlib.h>
#include <stdint.h>
#include "bignum.h"

#define SUBROUTINE_XLIST\
        X(subr_bignum_create,    "bignum")\
        X(subr_bignum_multiply,  "bignum-multiply")\
        X(subr_bignum_add,       "bignum-add")\
        X(subr_bignum_subtract,  "bignum-subtract")\
        X(subr_bignum_divide,    "bignum-divide")\
        X(subr_bignum_to_string, "bignum-to-string")

#define X(SUBR, NAME) static cell* SUBR (lisp *l, cell *args);
SUBROUTINE_XLIST /*function prototypes for all of the built-in subroutines*/
#undef X

#define X(SUBR, NAME) { SUBR, NAME },
static struct module_subroutines { subr p; char *name; } primitives[] = {
        SUBROUTINE_XLIST /*all of the subr functions*/
        {NULL, NULL} /*must be terminated with NULLs*/
};
#undef X

static intptr_t ud_bignum = 0;

static void ud_bignum_free(cell *f) { 
        bignum_destroy(userval(f));
        free(f);
}

static int ud_bignum_print(io *o, unsigned depth, cell *f) {
        int ret;
        char *s;
        s = bignum_bigtostr(userval(f), 10);
        ret = lisp_printf(NULL, o, depth, "%mb%s%t", s);
        free(s);
        return ret;
}

static cell* subr_bignum_create(lisp *l, cell *args) {
        cell *ret;
        if(!cklen(args, 1) || !is_int(car(args)))
                RECOVER(l, "\"expected (integer)\" '%S", args);
        if(!(ret = mk_user(l, (void*)bignum_create(intval(car(args)), 16), ud_bignum)))
                HALT(l, "\"%s\"", "out of memory");
        return ret;
}

static cell* subr_bignum_multiply(lisp *l, cell *args) {
        cell *ret;
        if(!cklen(args, 2) || !is_usertype(car(args), ud_bignum) || !is_usertype(CADR(args), ud_bignum))
                RECOVER(l, "\"expected (bignum bignum)\" '%S", args);
        if(!(ret = mk_user(l, (void*)bignum_multiply(userval(car(args)), userval(CADR(args))), ud_bignum)))
                HALT(l, "\"%s\"", "out of memory");
        return ret;
}

static cell* subr_bignum_add(lisp *l, cell *args) {
        cell *ret;
        if(!cklen(args, 2) || !is_usertype(car(args), ud_bignum) || !is_usertype(CADR(args), ud_bignum))
                RECOVER(l, "\"expected (bignum bignum)\" '%S", args);
        if(!(ret = mk_user(l, (void*)bignum_add(userval(car(args)), userval(CADR(args))), ud_bignum)))
                HALT(l, "\"%s\"", "out of memory");
        return ret;
}

static cell* subr_bignum_subtract(lisp *l, cell *args) {
        cell *ret;
        if(!cklen(args, 2) || !is_usertype(car(args), ud_bignum) || !is_usertype(CADR(args), ud_bignum))
                RECOVER(l, "\"expected (bignum bignum)\" '%S", args);
        if(!(ret = mk_user(l, (void*)bignum_subtract(userval(car(args)), userval(CADR(args))), ud_bignum)))
                HALT(l, "\"%s\"", "out of memory");
        return ret;
}

static cell* subr_bignum_divide(lisp *l, cell *args) {
        bignum_div_t *d;
        cell *ret;
        if(!cklen(args, 2) || !is_usertype(car(args), ud_bignum) || !is_usertype(CADR(args), ud_bignum))
                RECOVER(l, "\"expected (bignum bignum)\" '%S", args);
        if(!(d = bignum_divide(userval(car(args)), userval(CADR(args))), ud_bignum))
                HALT(l, "\"%s\"", "out of memory");
        ret = cons(l, mk_user(l, d->quotient, ud_bignum), mk_user(l, d->remainder, ud_bignum));
        free(d);
        return ret;
}

static cell* subr_bignum_to_string(lisp *l, cell *args) {
        char *s;
        if(!cklen(args, 1) || !is_usertype(car(args), ud_bignum))
                RECOVER(l, "\"expected (bignum)\" '%S", args);
        if(!(s = bignum_bigtostr(userval(car(args)), 10)))
                HALT(l, "\"%s\"", "out of memory");
        return mk_str(l, s);
}

static int initialize(void) {
        size_t i;
        assert(lglobal);

        ud_bignum = newuserdef(lglobal, ud_bignum_free, NULL, NULL, ud_bignum_print);
        for(i = 0; primitives[i].p; i++) /*add all primitives from this module*/
                if(!lisp_add_subr(lglobal, primitives[i].name, primitives[i].p))
                        goto fail;
        lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: bignum loaded\n");
        return 0;
fail:   lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: bignum load failure\n");
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__((constructor));
static void destruct(void)  __attribute__((destructor));
static void construct(void) { initialize(); }
static void destruct(void)  { }
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

