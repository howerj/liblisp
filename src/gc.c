/** @file       gc.c
 *  @brief      The garbage collector for liblisp
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com*/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <stdlib.h>

void lisp_gc_used(lisp_cell_t *x)
{
	assert(x);
	x->used = 1;
}

void lisp_gc_not_used(lisp_cell_t *x)
{
	assert(x);
	x->used = 0;
}

static void gc_free(lisp_t * l, lisp_cell_t * x)
{	
        /*assert(op) *//**< free a lisp cell*/
	if (!x || x->uncollectable || x->used)
		return;
	switch (x->type) {
	case INTEGER:
	case CONS:
	case FLOAT:
	case PROC:
	case SUBR:
	case FPROC:
		free(x);
		break;
	case STRING:
		free(get_str(x));
		free(x);
		break;
	case SYMBOL:
		free(get_sym(x));
		free(x);
		break;
	case IO:
		if (!x->close)
			io_close(get_io(x));
		free(x);
		break;
	case HASH:
		hash_destroy(get_hash(x));
		free(x);
		break;
	case USERDEF:
		if (l->ufuncs[get_user_type(x)].free)
			(l->ufuncs[get_user_type(x)].free) (x);
		else
			free(x);
		break;
	case INVALID:
	default:
		FATAL("internal inconsistency");
		break;
	}
}

void lisp_gc_mark(lisp_t * l, lisp_cell_t * op)
{						  
        /*assert(op); *//**<recursively mark reachable cells*/
	if (!op || op->mark)
		return;
	op->mark = 1;
	switch (op->type) {
	case INTEGER:
	case SYMBOL:
	case STRING:
	case IO:
	case FLOAT:
		break;
	case SUBR:
		lisp_gc_mark(l, get_func_docstring(op));
		break;
	case FPROC:
	case PROC:
		lisp_gc_mark(l, get_proc_args(op));
		lisp_gc_mark(l, get_proc_code(op));
		lisp_gc_mark(l, get_proc_env(op));
		lisp_gc_mark(l, get_func_docstring(op));
		break;
	case CONS:
		lisp_gc_mark(l, car(op));
		lisp_gc_mark(l, cdr(op));
		break;
	case HASH:{
			size_t i;
			hash_entry_t *cur;
			hash_table_t *h = get_hash(op);
			for (i = 0; i < h->len; i++)
				if (h->table[i])
					for (cur = h->table[i]; cur; cur = cur->next)
						lisp_gc_mark(l, cur->val);
		}
		break;
	case USERDEF:
		if (l->ufuncs[get_user_type(op)].mark)
			(l->ufuncs[get_user_type(op)].mark) (op);
		break;
	case INVALID:
	default:
		FATAL("internal inconsistency: unknown type");
	}
}

void lisp_gc_sweep_only(lisp_t * l)
{				
	gc_list_t *v, **p;
	if (l->gc_off)
		return;
	for (p = &l->gc_head; *p != NULL;) {
		v = *p;
		if (v->ref->mark) {
			p = &v->next;
			v->ref->mark = 0;
		} else {
			*p = v->next;
			gc_free(l, v->ref);
			free(v);
		}
	}
}

lisp_cell_t *lisp_gc_add(lisp_t * l, lisp_cell_t * op)
{				  
	lisp_cell_t **olist;
	if (l->gc_stack_used++ > l->gc_stack_allocated - 1) {
		l->gc_stack_allocated = l->gc_stack_used * 2;
		if (l->gc_stack_allocated < l->gc_stack_used)
			LISP_HALT(l, "%s", "overflow in allocator size variable");
		olist = realloc(l->gc_stack, l->gc_stack_allocated * sizeof(*l->gc_stack));
		if (!olist)
			LISP_HALT(l, "%s", "out of memory");
		l->gc_stack = olist;
	}
	l->gc_stack[l->gc_stack_used - 1] = op;	/**<anything reachable in here is not freed*/
	return op;
}

int lisp_gc_status(lisp_t * l)
{
	return !l->gc_off;
}

void lisp_gc_on(lisp_t * l)
{
	l->gc_off = 0;
}

void lisp_gc_off(lisp_t * l)
{
	l->gc_off = 1;
}

void lisp_gc_mark_and_sweep(lisp_t * l)
{
	size_t i;
	if (l->gc_off)
		return;
	lisp_gc_mark(l, l->all_symbols);
	lisp_gc_mark(l, l->top_env);
	for (i = 0; i < l->gc_stack_used; i++)
		lisp_gc_mark(l, l->gc_stack[i]);
	lisp_gc_sweep_only(l);
	l->gc_collectp = 0;
}

