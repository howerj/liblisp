/** @file       gc.c
 *  @brief      The garbage collector for liblisp
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com*/

#include "liblisp.h"
#include "private.h"
#include <stdlib.h>

static void gc_free(lisp *l, cell *ob) { /**< free a lisp cell*/
        if(ob->uncollectable) return;
        switch(ob->type) {
        case INTEGER: case CONS: case FLOAT:
        case PROC:    case SUBR: case FPROC:      free(ob); break;
        case STRING:  free(strval(ob));           free(ob); break;
        case SYMBOL:  free(symval(ob));           free(ob); break;
        case IO:      if(!ob->close)   io_close(ioval(ob));        
                                                  free(ob); break; 
        case HASH:    hash_destroy(hashval(ob));  free(ob); break;
        case USERDEF: if(l->ufuncs[ob->userdef].free)
                              (l->ufuncs[ob->userdef].free)(ob);
                      else free(ob);
                      break;
        case INVALID:
        default: FATAL("internal inconsistency"); break;
        }
}

static void gc_mark(lisp *l, cell* op) { /**<recursively mark reachable cells*/
        if(l->gc_state != GC_ON) return;
        if(!op || op->uncollectable || op->mark) return;
        op->mark = 1;
        switch(op->type) {
        case INTEGER: case SYMBOL: case SUBR: 
        case STRING:  case IO:     case FLOAT:  break;
        case FPROC: case PROC: 
                   gc_mark(l, procargs(op)); 
                   gc_mark(l, proccode(op));
                   gc_mark(l, procenv(op));
                   break;
        case CONS: gc_mark(l, car(op));
                   gc_mark(l, cdr(op));
                   break;
        case HASH: {
                        size_t i;
                        hashentry *cur;
                        hashtable *h = hashval(op);
                        for(i = 0; i < h->len; i++)
                                if(h->table[i])
                                        for(cur = h->table[i]; cur; cur = cur->next)
                                                gc_mark(l, cur->val);
                   }
                   break;
        case USERDEF: if(l->ufuncs[op->userdef].mark)
                             (l->ufuncs[op->userdef].mark)(op);
                      break;
        case INVALID:
        default:   FATAL("internal inconsistency: unknown type");
        }
}

void gc_sweep_only(lisp *l) { /*linked list versus dynamic array?*/
        gc_list *v, **p;
        if(l->gc_state != GC_ON) return;
        for(p = &l->gc_head; *p != NULL;) { 
                v = *p;
                if(v->ref->mark) {
                        p = &v->next;
                        v->ref->mark = 0;
                } else {
                        *p = v->next;
                        gc_free(l, v->ref);
                        free(v);
                }
        }
}

cell *gc_add(lisp *l, cell* op) { /**< add a cell to the working set*/
        cell **olist;
        if(l->gc_state == GC_OFF) return op;
        if(l->gc_stack_used++ > l->gc_stack_allocated - 1) {
                l->gc_stack_allocated = l->gc_stack_used * 2;
                if(l->gc_stack_allocated < l->gc_stack_used) 
                        HALT(l, "%s", "overflow in allocator size variable");
                olist = realloc(l->gc_stack, l->gc_stack_allocated * 
                                sizeof(*l->gc_stack));
                if(!olist)
                        HALT(l, "%s", "out of memory");
                l->gc_stack = olist;
        }
        l->gc_stack[l->gc_stack_used - 1] = op; /**<anything reachable in here is not freed*/
        return op;
}

void gc_mark_and_sweep(lisp *l) {
        size_t i;
        gc_mark(l, l->all_symbols);
        gc_mark(l, l->top_env);
        for(i = 0; i < l->gc_stack_used; i++)
                gc_mark(l, l->gc_stack[i]);
        gc_sweep_only(l);
        l->gc_collectp = 0;
}

