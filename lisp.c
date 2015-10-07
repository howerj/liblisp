/** @file       lisp.c
 *  @brief      A minimal lisp interpreter and utility library
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com**/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

lisp *lglobal; /**< used for signal handler and modules*/

void lisp_throw(lisp *l, int ret) {
        if(!l->errors_halt && l && l->recover_init) longjmp(l->recover, ret);
        else exit(ret);
}

cell *lisp_add_subr(lisp *l, char *name, subr func) { assert(l && func && name);
        return extend_top(l, intern(l, lstrdup(name)), mk_subr(l, func));
}

cell *lisp_intern(lisp *l, cell *ob) { assert(l && ob);
        if(hash_insert(hashval(l->all_symbols), symval(ob) , ob)) return NULL;
        return l->tee;
}

cell *lisp_add_cell(lisp *l, char *sym, cell *val) { assert(l && sym && val);
        return extend_top(l, intern(l, lstrdup(sym)), val);
}

void lisp_destroy(lisp *l) {
        if(!l) return;
        if(l->gc_stack) gc_sweep_only(l), free(l->gc_stack);
        if(l->buf) free(l->buf);
        if(l->efp) io_close(l->efp);
        if(l->ofp) io_close(l->ofp);
        if(l->ifp) io_close(l->ifp);
        free(l);
}

cell *lisp_read(lisp *l, io *i) { assert(l && i);
        cell *ret;
        int restore_used, r;
        jmp_buf restore;
        if(l->recover_init) {
                memcpy(restore, l->recover, sizeof(jmp_buf));
                restore_used = 1;
        }
        if((r = setjmp(l->recover))) { 
                RECOVER_RESTORE(restore_used, l, restore); 
                return r > 0 ? l->error : NULL;
        }
        l->recover_init = 1;
        ret = reader(l, i);
        RECOVER_RESTORE(restore_used, l, restore); 
        return ret;
}

int lisp_print(lisp *l, cell *ob) { assert(l && ob);
        int ret = printer(l, l->ofp, ob, 0);
        io_putc('\n', l->ofp);
        return ret;
}

cell *lisp_eval(lisp *l, cell *exp) { assert(l && exp);
        cell *ret;
        int restore_used, r;
        jmp_buf restore;
        if(l->recover_init) {
                memcpy(restore, l->recover, sizeof(jmp_buf));
                restore_used = 1;
        }
        if((r = setjmp(l->recover))) {
                RECOVER_RESTORE(restore_used, l, restore); 
                return r > 0 ? l->error : NULL;
        }
        l->recover_init = 1;
        ret = eval(l, 0, exp, l->top_env);
        RECOVER_RESTORE(restore_used, l, restore); 
        return ret;
}

cell *lisp_eval_string(lisp *l, char *evalme) { assert(l && evalme);
        io *in = NULL;
        cell *ret;
        volatile int restore_used = 0, r;
        jmp_buf restore;
        if(!(in = io_sin(evalme))) return NULL;
        if(l->recover_init) {
                memcpy(restore, l->recover, sizeof(jmp_buf));
                restore_used = 1;
        }
        if((r = setjmp(l->recover))) {
                io_close(in);
                RECOVER_RESTORE(restore_used, l, restore); 
                return r > 0 ? l->error : NULL;
        }
        l->recover_init = 1;
        ret = eval(l, 0, reader(l, in), l->top_env);
        io_close(in);
        RECOVER_RESTORE(restore_used, l, restore); 
        return ret;
}

int lisp_set_input(lisp *l, io *in) { assert(l && in);
        if(!io_is_in(in)) return -1;
        l->ifp = in;
        return 0;
}

int lisp_set_output(lisp *l, io *out) { assert(l && out);
        if(!io_is_out(out)) return -1;
        l->ofp = out;
        return 0;
}

int lisp_set_logging(lisp *l, io *logging) { assert(l && logging);
        if(!io_is_out(logging)) return -1;
        l->efp = logging;
        return 0;
}

void lisp_set_line_editor(lisp *l, editor_func ed){ assert(l); l->editor = ed; }
void lisp_set_signal(lisp *l, int sig) { assert(l); l->sig = sig; }

io *lisp_get_input(lisp *l)   { assert(l); return l->ifp; }
io *lisp_get_output(lisp *l)  { assert(l); return l->ofp; }
io *lisp_get_logging(lisp *l) { assert(l); return l->efp; }

int  lisp_is_cell_closed(cell *f) { assert(f); return f->close; }
void lisp_close_cell(cell *f) { assert(f); f->close = 1; }
int  lisp_get_cell_length(cell *c) { assert(c); return c->len; }

