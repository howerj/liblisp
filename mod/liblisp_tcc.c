/** @file       liblisp_tcc.c
 *  @brief      Tiny C Compiler liblisp module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com**/
#include <assert.h>
#include <libtcc.h>
#include <liblisp.h>
#include <stdlib.h>

extern lisp *lglobal; /* from main.c */

static void construct(void) __attribute__((constructor));
static void destruct(void) __attribute__((destructor));

#define SUBROUTINE_XLIST\
        X(subr_compile,             "compile")\
        X(subr_link,                "link-library")\
        X(subr_compile_file,        "compile-file")\
        X(subr_get_subr,            "get-subroutine")\
        X(subr_add_include_path,    "add-include-path")\
        X(subr_add_sysinclude_path, "add-system-include-path")\
        X(subr_set_lib_path,        "set-library-path")\


#define X(SUBR, NAME) static cell* SUBR (lisp *l, cell *args);
SUBROUTINE_XLIST /*function prototypes for all of the built-in subroutines*/
#undef X

#define X(SUBR, NAME) { SUBR, NAME },
static struct module_subroutines { subr p; char *name; } primitives[] = {
        SUBROUTINE_XLIST /*all of the subr functions*/
        {NULL, NULL} /*must be terminated with NULLs*/
};
#undef X

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
        || !is_usertype(car(args), ud_tcc)
        || !is_asciiz(CADR(args)) || !is_str(CADDR(args)))
                RECOVER(l, "\"expected (compile-state string string\" '%S", args);
        char *fname = strval(CADR(args)), *prog = strval(CADDR(args));
        subr func;
        TCCState *st = userval(car(args));
        if(tcc_compile_string(st, prog) < 0)
                return gsym_error();
        if(tcc_relocate(st, TCC_RELOCATE_AUTO) < 0)
                return gsym_error();
        func = (subr)tcc_get_symbol(st, fname);
        return mksubr(l, func);
}

static cell* subr_link(lisp *l, cell *args) {
        if(!cklen(args, 2) 
        || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (compile-state string)\" '%S", args);
        return tcc_add_library(userval(car(args)), strval(CADR(args))) < 0 ? gsym_error() : gsym_nil();
}

static cell* subr_compile_file(lisp *l, cell *args) {
        if(!cklen(args, 2)
        || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (compile-state string)\" '%S", args);
        if(tcc_add_file(userval(car(args)), strval(CADR(args))) < 0)
                return gsym_error();
        if(tcc_relocate(userval(car(args)), TCC_RELOCATE_AUTO) < 0)
                return gsym_error();
        return gsym_tee();
}

static cell* subr_get_subr(lisp *l, cell *args) {
        subr func;
        if(!cklen(args, 2)
        || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (compile-state string)\" '%S", args);
        if(!(func = tcc_get_symbol(userval(car(args)), strval(CADR(args)))))
                return gsym_error();
        else
                return mksubr(l, func);
}

static cell* subr_add_include_path(lisp *l, cell *args) {
        if(!cklen(args, 2) || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (compile-state string)\" '%S", args);
        return tcc_add_include_path(userval(car(args)), strval(CADR(args))) < 0 ? gsym_error() : gsym_tee();
}

static cell* subr_add_sysinclude_path(lisp *l, cell *args) {
        if(!cklen(args, 2) || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (compile-state string)\" '%S", args);
        return tcc_add_sysinclude_path(userval(car(args)), strval(CADR(args))) < 0 ? gsym_error() : gsym_tee();
}

static cell* subr_set_lib_path(lisp *l, cell *args) {
        if(!cklen(args, 2) || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (compile-state string)\" '%S", args);
        tcc_set_lib_path(userval(car(args)), strval(CADR(args)));
        return gsym_tee();
}

static void construct(void) {
        size_t i;
        assert(lglobal);
        /** Tiny C compiler library interface, special care has to be taken 
         *  when compiling and linking all of the C files within the liblisp
         *  project so the symbols in it are available to libtcc.
         *
         *  Possible improvements:
         *  * Modification of libtcc so it can accept S-Expressions from
         *    the interpreter. This would be a significant undertaking.
         *  * Add more functions from libtcc
         *  * Separate out tcc_get_symbol from tcc_compile_string
         *  * Find out why link does not work
         **/
        ud_tcc = newuserdef(lglobal, ud_tcc_free, NULL, NULL, ud_tcc_print);
        TCCState *st = tcc_new();
        tcc_set_output_type(st, TCC_OUTPUT_MEMORY);
        lisp_add_cell(lglobal, "*compile-state*", mkuser(lglobal, st, ud_tcc));
        for(i = 0; primitives[i].p; i++) /*add all primitives from this module*/
                if(!lisp_add_subr(lglobal, primitives[i].name, primitives[i].p))
                        goto fail;
        printerf(lglobal, lisp_get_logging(lglobal), 0, "module: TCC loaded\n");
        return;
fail:   printerf(lglobal, lisp_get_logging(lglobal), 0, "module: TCC load failure\n");
}

static void destruct(void) {
}
