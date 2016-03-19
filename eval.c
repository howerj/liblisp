/** @file       eval.c
 *  @brief      The evaluator for the lisp interpreter
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *
 *  This is the main evaluator and associated function, the built in
 *  subroutines for the interpreter are defined elsewhere. **/
#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static const int dynamic_on = 0; /**< 0 for lexical scoping, !0 for dynamic scoping*/

static lisp_cell_t *mk(lisp_t * l, lisp_type type, size_t count, ...)
{ /**@brief make new lisp cells and perform garbage bookkeeping/collection*/
	assert(l && type != INVALID && count);
	lisp_cell_t *ret;
	gc_list_t *node; /**< new node in linked list of all allocations*/
	va_list ap;
	size_t i;

	if (l->gc_collectp++ > COLLECTION_POINT)	/*set to 1 for testing */
		lisp_gc_mark_and_sweep(l);

	va_start(ap, count);
	if (!(ret = calloc(1, sizeof(lisp_cell_t) + (count - 1) * sizeof(cell_data_t))))
		LISP_HALT(l, "\"%s\"", "out of memory");
	if (!(node = calloc(1, sizeof(*node))))
		LISP_HALT(l, "\"%s\"", "out of memory");
	ret->type = type;
	for (i = 0; i < count; i++)
		if (FLOAT == type)
			ret->p[i].f = va_arg(ap, double);
		else if (SUBR == type)
			ret->p[i].prim = va_arg(ap, lisp_subr_func);
		else
			ret->p[i].v = va_arg(ap, void *);
	va_end(ap);
	node->ref = ret;
	node->next = l->gc_head;
	l->gc_head = node;
	lisp_gc_add(l, ret);
	return ret;
}

lisp_cell_t *cons(lisp_t * l, lisp_cell_t * x, lisp_cell_t * y)
{
	assert(l);
	return mk(l, CONS, 2, x, y);
}

lisp_cell_t *car(lisp_cell_t * con)
{
	assert(con && is_cons(con));
	return con->p[0].v;
}

lisp_cell_t *cdr(lisp_cell_t * con)
{
	assert(con && is_cons(con));
	return con->p[1].v;
}

void set_car(lisp_cell_t * con, lisp_cell_t * val)
{
	assert(con && is_cons(con) && val);
	con->p[0].v = val;
}

void set_cdr(lisp_cell_t * con, lisp_cell_t * val)
{
	assert(con && is_cons(con) && val);
	con->p[1].v = val;
}

void close_cell(lisp_cell_t * x)
{
	assert(x);
	x->close = 1;
}

int lisp_check_length(lisp_cell_t * x, size_t expect)
{
	assert(x);
	return get_length(x) == expect;
}

int is_nil(lisp_cell_t * x)
{
	assert(x);
	return x == gsym_nil();
}

int is_int(lisp_cell_t * x)
{
	assert(x);
	return x->type == INTEGER;
}

int is_floating(lisp_cell_t * x)
{
	assert(x);
	return x->type == FLOAT;
}

int is_io(lisp_cell_t * x)
{
	assert(x);
	return x->type == IO && !x->close;
}

int is_cons(lisp_cell_t * x)
{
	assert(x);
	return x->type == CONS;
}

int is_proper_cons(lisp_cell_t * x)
{
	assert(x);
	return is_cons(x) && (is_nil(cdr(x)) || is_cons(cdr(x)));
}

int is_proc(lisp_cell_t * x)
{
	assert(x);
	return x->type == PROC;
}

int is_fproc(lisp_cell_t * x)
{
	assert(x);
	return x->type == FPROC;
}

int is_str(lisp_cell_t * x)
{
	assert(x);
	return x->type == STRING;
}

int is_sym(lisp_cell_t * x)
{
	assert(x);
	return x->type == SYMBOL;
}

int is_subr(lisp_cell_t * x)
{
	assert(x);
	return x->type == SUBR;
}

int is_hash(lisp_cell_t * x)
{
	assert(x);
	return x->type == HASH;
}

int is_userdef(lisp_cell_t * x)
{
	assert(x);
	return x->type == USERDEF && !x->close;
}

int is_usertype(lisp_cell_t * x, int type)
{
	assert(x && type < MAX_USER_TYPES && type >= 0);
	return x->type == USERDEF && get_user_type(x) == type && !x->close;
}

int is_asciiz(lisp_cell_t * x)
{
	assert(x);
	return is_str(x) || is_sym(x);
}

int is_arith(lisp_cell_t * x)
{
	assert(x);
	return is_int(x) || is_floating(x);
}

int is_func(lisp_cell_t * x)
{
	assert(x);
	return is_proc(x) || is_fproc(x) || is_subr(x);
}

int is_closed(lisp_cell_t * x)
{
	assert(x);
	return x->close;
}

int is_list(lisp_cell_t * x)
{
	assert(x);
	for (; !is_nil(x); x = cdr(x))
		if (!is_cons(cdr(x)) && !is_nil(cdr(x)))
			return 0;
	return 1;
}

static lisp_cell_t *mk_asciiz(lisp_t * l, char *s, lisp_type type)
{
	assert(l && s && (type == STRING || type == SYMBOL));
	lisp_cell_t *x = mk(l, type, 2, (lisp_cell_t *) s, strlen(s));
	return x;
}

static lisp_cell_t *mk_sym(lisp_t * l, char *s)
{
	return mk_asciiz(l, s, SYMBOL);
}

lisp_cell_t *mk_list(lisp_t * l, lisp_cell_t * x, ...)
{
	assert(x);
	size_t i;
	lisp_cell_t *head, *op, *next;
	va_list ap;
	head = op = cons(l, x, gsym_nil());
	va_start(ap, x);
	for (i = 1; (next = va_arg(ap, lisp_cell_t *)); op = cdr(op), i++)
		set_cdr(op, cons(l, next, gsym_nil()));
	va_end(ap);
	return head;
}

lisp_cell_t *mk_int(lisp_t * l, intptr_t d)
{
	assert(l);
	return mk(l, INTEGER, 1, (lisp_cell_t *) d);
}

lisp_cell_t *mk_io(lisp_t * l, io_t * x)
{
	assert(l && x);
	return mk(l, IO, 1, (lisp_cell_t *) x);
}

lisp_cell_t *mk_subr(lisp_t * l, lisp_subr_func p, const char *fmt, const char *doc)
{
	assert(l && p);
	lisp_cell_t *t;
	t = mk(l, SUBR, 4, p, NULL, NULL, NULL);
	if (fmt) {
		size_t tlen = lisp_validate_arg_count(fmt);
		assert((BITS_IN_LENGTH >= 32) && tlen < 0xFFFFFFFFu);
		t->p[3].v = (void*)tlen;
	}
	t->p[1].v = (void *)fmt;
	t->p[2].v = (void *)mk_str(l, lisp_strdup(l, doc ? doc : ""));
	return t;
}

lisp_cell_t *mk_proc(lisp_t * l, lisp_cell_t * args, lisp_cell_t * code, lisp_cell_t * env, lisp_cell_t * doc)
{
	assert(l && args && code && env);
	return mk(l, PROC, 5, args, code, env, NULL, doc);
}

lisp_cell_t *mk_fproc(lisp_t * l, lisp_cell_t * args, lisp_cell_t * code, lisp_cell_t * env, lisp_cell_t * doc)
{
	assert(l && args && code && env);
	return mk(l, FPROC, 5, args, code, env, NULL, doc);
}

lisp_cell_t *mk_float(lisp_t * l, lisp_float_t f)
{
	assert(l);
	return mk(l, FLOAT, 1, f);
}

lisp_cell_t *mk_str(lisp_t * l, char *s)
{ /**@todo fix for binary data, also make version that automatically does the string dup*/
	return mk_asciiz(l, s, STRING);
}

lisp_cell_t *mk_immutable_str(lisp_t * l, const char *s) 
{
	lisp_cell_t *r = mk_str(l, (char*)s);
	r->uncollectable = 1;
	return r;
}

lisp_cell_t *mk_hash(lisp_t * l, hash_table_t * h)
{
	return mk(l, HASH, 1, (lisp_cell_t *) h);
}

lisp_cell_t *mk_user(lisp_t * l, void *x, intptr_t type)
{
	assert(l && x && type >= 0 && type < l->user_defined_types_used);
	lisp_cell_t *ret = mk(l, USERDEF, 2, x);
	ret->p[1].v = (void *)type;
	return ret;
}

unsigned get_length(lisp_cell_t * x)
{ 
	size_t i;
	assert(x);
	if(is_nil(x))
		return 0;
	switch(x->type) {
	case STRING:
	case SYMBOL:
		return (uintptr_t)(x->p[1].v);
	case CONS:
		for(i = 0; is_cons(x); x = cdr(x))
			i++;
		return i;
	case SUBR:
		return (uintptr_t)(x->p[3].v);
	default:
		return 0;
	}
}

void *get_raw(lisp_cell_t * x)
{
	assert(x);
	return x->p[0].v;
}

intptr_t get_int(lisp_cell_t * x)
{
	return !x ? 0 : (intptr_t) (x->p[0].v);
}

lisp_subr_func get_subr(lisp_cell_t * x)
{
	assert(x && is_subr(x));
	return x->p[0].prim;
}

lisp_cell_t *get_proc_args(lisp_cell_t * x)
{
	assert(x && (is_proc(x) || is_fproc(x)));
	return x->p[0].v;
}

lisp_cell_t *get_proc_code(lisp_cell_t * x)
{
	assert(x && (is_proc(x) || is_fproc(x)));
	return x->p[1].v;
}

lisp_cell_t *get_proc_env(lisp_cell_t * x)
{
	assert(x && (is_proc(x) || is_fproc(x)));
	return x->p[2].v;
}

lisp_cell_t *get_func_docstring(lisp_cell_t * x)
{
	assert(x && is_func(x));
	return is_subr(x) ? x->p[2].v : x->p[4].v;
}

char *get_func_format(lisp_cell_t * x)
{
	assert(x && is_func(x));
	return is_subr(x) ? x->p[1].v : x->p[3].v;
}

io_t *get_io(lisp_cell_t * x)
{
	assert(x && x->type == IO);
	return (io_t *) (x->p[0].v);
}

char *get_sym(lisp_cell_t * x)
{
	assert(x && is_asciiz(x));
	return (char *)(x->p[0].v);
}

char *get_str(lisp_cell_t * x)
{
	assert(x && is_asciiz(x));
	return (char *)(x->p[0].v);
}

void *get_user(lisp_cell_t * x)
{
	assert(x && x->type == USERDEF);
	return (void *)(x->p[0].v);
}

int get_user_type(lisp_cell_t * x)
{
	assert(x && x->type == USERDEF);
	return (intptr_t) x->p[1].v;
}

hash_table_t *get_hash(lisp_cell_t * x)
{
	assert(x && is_hash(x));
	return (hash_table_t *) (x->p[0].v);
}

lisp_float_t get_float(lisp_cell_t * x)
{
	assert(x && is_floating(x));
	return x->p[0].f;
}

intptr_t get_a2i(lisp_cell_t * x)
{
	assert(x && is_arith(x));
	return is_int(x) ? get_int(x) : (intptr_t) get_float(x);
}

lisp_float_t get_a2f(lisp_cell_t * x)
{
	assert(x && is_arith(x));
	return is_floating(x) ? get_float(x) : (lisp_float_t) get_int(x);
}

int is_in(lisp_cell_t * x)
{
	assert(x);
	return (x && is_io(x) && io_is_in(get_io(x))) ? 1 : 0;
}

int is_out(lisp_cell_t * x)
{
	assert(x);
	return (x && is_io(x) && io_is_out(get_io(x))) ? 1 : 0;
}

int new_user_defined_type(lisp_t * l, lisp_free_func f, lisp_mark_func m, lisp_equal_func e, lisp_print_func p)
{
	if (l->user_defined_types_used >= MAX_USER_TYPES)
		return -1;
	l->ufuncs[l->user_defined_types_used].free = f;
	l->ufuncs[l->user_defined_types_used].mark = m;
	l->ufuncs[l->user_defined_types_used].equal = e;
	l->ufuncs[l->user_defined_types_used].print = p;
	return l->user_defined_types_used++;
}

lisp_cell_t *lisp_extend(lisp_t * l, lisp_cell_t * env, lisp_cell_t * sym, lisp_cell_t * val)
{
	return cons(l, cons(l, sym, val), env);
}

lisp_cell_t *lisp_intern(lisp_t * l, char *name)
{
	assert(l && name);
	lisp_cell_t *op = hash_lookup(get_hash(l->all_symbols), name);
	if (op)
		return op;
	op = mk_sym(l, name);
	hash_insert(get_hash(l->all_symbols), name, op);
	return op;
}

/***************************** environment ************************************/

static lisp_cell_t *multiple_extend(lisp_t * l, lisp_cell_t * env, lisp_cell_t * syms, lisp_cell_t * vals)
{
	assert(l && env && syms && vals);
	for (; !is_nil(syms); syms = cdr(syms), vals = cdr(vals))
		env = lisp_extend(l, env, car(syms), car(vals));
	return env;
}

lisp_cell_t *lisp_extend_top(lisp_t * l, lisp_cell_t * sym, lisp_cell_t * val)
{
	assert(l && sym && val);
	if (hash_insert(get_hash(l->top_hash), get_str(sym), cons(l, sym, val)) < 0)
		LISP_HALT(l, "\"%s\"", "out of memory");
	return val;
}

/**@todo an rassoc, that would search for a value and return a key, would be
 *       very useful*/
lisp_cell_t *lisp_assoc(lisp_cell_t * key, lisp_cell_t * alist)
{
	assert(key && alist);
	lisp_cell_t *lookup;
	for (; !is_nil(alist); alist = cdr(alist))
		if (is_cons(car(alist))) {	/*normal assoc */
			if (get_int(CAAR(alist)) == get_int(key))
				return car(alist);
		} else if (is_hash(car(alist)) && is_asciiz(key)) {	/*assoc extended with hashes */
			if ((lookup = hash_lookup(get_hash(car(alist)), get_str(key))))
				return lookup;
		}
	return gsym_nil();
}

/******************************** evaluator ***********************************/

/** @brief "Compile" an expression, that is, perform optimizations
 *         and bind as many variables as possible. This function
 *         is a work in progress.
 *  @bug  This function really, really needs fixing.
 *  @bug  F-Expressions should be prevent compilation of their arguments
 **/
static lisp_cell_t *binding_lambda(lisp_t * l, unsigned depth, lisp_cell_t * exp, lisp_cell_t * env)
{
	size_t i;
	lisp_cell_t *head, *op, *tmp, *code = gsym_nil();
	if (depth > MAX_RECURSION_DEPTH)
		LISP_RECOVER(l, "%y'recursion-depth-reached%t %d", depth);

	if (is_sym(car(exp)) && !is_nil(tmp = lisp_assoc(car(exp), env)))
		op = cdr(tmp);
	else if (is_cons(car(exp)))
		op = binding_lambda(l, depth + 1, car(exp), env);
	else
		op = car(exp);

	head = op = cons(l, op, gsym_nil());
	exp = cdr(exp);
	for (i = 1; is_cons(exp); exp = cdr(exp), op = cdr(op), i++) {
		code = car(exp);
		if (is_sym(car(exp)) && !is_nil(tmp = lisp_assoc(car(exp), env)))
			code = cdr(tmp);
		if (is_cons(car(exp)))
			code = binding_lambda(l, depth + 1, car(exp), env);
		set_cdr(op, cons(l, code, gsym_nil()));
	}
	if(!is_nil(exp))
		LISP_RECOVER(l, "%r\"compile cannot eval dotted pairs\"%t\n '%S", head);
	return head;
}

static lisp_cell_t *evlis(lisp_t * l, unsigned depth, lisp_cell_t * exps, lisp_cell_t * env);
lisp_cell_t *eval(lisp_t * l, unsigned depth, lisp_cell_t * exp, lisp_cell_t * env)
{
	assert(l);
	size_t gc_stack_save = l->gc_stack_used;
	lisp_cell_t *tmp, *first, *proc, *ret = NULL, *vals = l->nil;
#define DEBUG_RETURN(EXPR) do { ret = (EXPR); goto debug; } while(0);
	if (depth > MAX_RECURSION_DEPTH)
		LISP_RECOVER(l, "%y'recursion-depth-reached%t %d", 0);
	lisp_gc_add(l, exp);
	lisp_gc_add(l, env);
 tail:
	if (!exp || !env) /*happens when s-expression parser has an error*/
		return NULL;
	lisp_log_debug(l, "%y'eval%t '%S", exp);
	if (is_nil(exp))
		return gsym_nil();
	if (l->sig) {
		lisp_log_debug(l, "%y'eval%t 'signal-caught %d", (intptr_t)l->sig);
		l->sig = 0;
		lisp_throw(l, 1);
	}

	switch (exp->type) {
	case INTEGER:
	case SUBR:
	case PROC:
	case STRING:
	case FLOAT:
	case IO:
	case HASH:
	case FPROC:
	case USERDEF:
		return exp;	/*self evaluating types */
	case SYMBOL:
		/* checks could be added here so special forms are not looked
		 * up, but only if this improves the speed of things*/
		if (is_nil(tmp = lisp_assoc(exp, env)))
			LISP_RECOVER(l, "%r\"unbound symbol\"%t\n '%s", get_sym(exp));
		DEBUG_RETURN(cdr(tmp));
	case CONS:
		first = car(exp);
		exp = cdr(exp);

		/**@todo I might want to create a field in certain types so
		 * that they can be evaluated.
		 *
		 * Example:
		 *
		 * (SPECIAL-HASH "string") => looks up value in hash
		 * (SPECIAL-INPUT-PORT) => return line of text as string
		 *
		 * This could be done using references, the reference would
		 * need to prevent collection of what was being pointed to. 
		 */
		if (!is_nil(exp) && !is_proper_cons(exp))
			LISP_RECOVER(l, "%y'evaluation\n %r\"cannot eval dotted pair\"%t\n '%S", exp);
		if (is_cons(first))
			first = eval(l, depth + 1, first, env);
		if (first == l->iif) {
			LISP_VALIDATE_ARGS(l, "if", 3, "A A A", exp, 1);
			exp = !is_nil(eval(l, depth + 1, car(exp), env)) ? CADR(exp) : CADDR(exp);
			goto tail;
		}
		if (first == l->lambda) {
			lisp_cell_t *doc;
			if (get_length(exp) < 2)
				LISP_RECOVER(l, "%y'lambda\n %r\"argc < 2\"%t\n '%S\"", exp);
			if (!is_nil(car(exp)) && is_str(car(exp))) {	/*have docstring */
				doc = car(exp);
				exp = cdr(exp);
			} else {
				doc = l->empty_docstr;
			}
			if (!is_nil(car(exp)) && !is_cons(car(exp)))
				LISP_RECOVER(l, "'lambda\n \"not an argument list (or nil)\"\n '%S", exp);
			for (tmp = car(exp); !is_nil(tmp); tmp = cdr(tmp))
				if (!is_sym(car(tmp)) || !is_proper_cons(tmp))
					LISP_RECOVER(l, "%y'lambda\n %r\"expected only symbols (or nil) as arguments\"%t\n '%S", exp);
			l->gc_stack_used = gc_stack_save;
			tmp = mk_proc(l, car(exp), cdr(exp), env, doc);
			DEBUG_RETURN(lisp_gc_add(l, tmp));
		}
		if (first == l->flambda) {
			if (get_length(exp) < 3 || !is_str(car(exp)) || !is_cons(CADR(exp)))
				LISP_RECOVER(l, "%y'flambda\n %r\"expected (string (arg) code...)\"%t\n '%S", exp);
			if (!lisp_check_length(CADR(exp), 1) || !is_sym(car(CADR(exp))))
				LISP_RECOVER(l, "%y'flambda\n %r\"only one symbol argument allowed\"%t\n '%S", exp);
			l->gc_stack_used = gc_stack_save;
			DEBUG_RETURN(lisp_gc_add(l, mk_fproc(l, CADR(exp), CDDR(exp), env, car(exp))));
		}
		if (first == l->cond) {
			if (lisp_check_length(exp, 0))
				DEBUG_RETURN(l->nil);
			for (tmp = l->nil; is_nil(tmp) && !is_nil(exp); exp = cdr(exp)) {
				if (!is_cons(car(exp)))
					DEBUG_RETURN(l->nil);
				tmp = eval(l, depth + 1, CAAR(exp), env);
				if (!is_nil(tmp)) {
					exp = CADAR(exp);
					goto tail;
				}
			}
			DEBUG_RETURN(l->nil);
		}
		if (first == l->quote)
			DEBUG_RETURN(car(exp));
		if (first == l->define) {
			LISP_VALIDATE_ARGS(l, "define", 2, "s A", exp, 1);
			l->gc_stack_used = gc_stack_save;
			DEBUG_RETURN(lisp_gc_add(l, lisp_extend_top(l, car(exp), eval(l, depth + 1, CADR(exp), env))));
		}
		if (first == l->set) {
			lisp_cell_t *pair, *newval;
			LISP_VALIDATE_ARGS(l, "set!", 2, "s A", exp, 1);
			if (is_nil(pair = lisp_assoc(car(exp), env)))
				LISP_RECOVER(l, "%y'set!\n %r\"undefined variable\"%t\n '%S", exp);
			newval = eval(l, depth + 1, CADR(exp), env);
			set_cdr(pair, newval);
			DEBUG_RETURN(newval);
		}
		if (first == l->compile) {
			lisp_cell_t *doc;
			LISP_VALIDATE_ARGS(l, "compile", 3, "Z L c", exp, 1);
			doc = car(exp);
			for (tmp = CADR(exp); !is_nil(tmp); tmp = cdr(tmp))
				if (!is_sym(car(tmp)) || !is_proper_cons(tmp))
					LISP_RECOVER(l, "%y'lambda\n %r\"expected only symbols (or nil) as arguments\"%t\n %S", exp);
			tmp = binding_lambda(l, depth + 1, CADDR(exp), env);
			DEBUG_RETURN(mk_proc(l, CADR(exp), cons(l, tmp, gsym_nil()), env, doc));
		}
		if (first == l->let) {
			lisp_cell_t *r = NULL, *s = NULL;
			if (get_length(exp) < 2)
				LISP_RECOVER(l, "%y'let\n %r\"argc < 2\"%t\n '%S", exp);
			tmp = exp;
			for (; !is_nil(cdr(exp)); exp = cdr(exp)) {
				if (!is_cons(car(exp)) || !lisp_check_length(car(exp), 2))
					LISP_RECOVER(l, "%y'let\n %r\"expected list of length 2\"%t\n '%S\n '%S", car(exp), tmp);
				s = env = lisp_extend(l, env, CAAR(exp), l->nil);
				r = env = lisp_extend(l, env, CAAR(exp), eval(l, depth + 1, CADAR(exp), env));
				set_cdr(car(s), CDAR(r));
			}
			DEBUG_RETURN(eval(l, depth + 1, car(exp), env));
		}
		if (first == l->progn) {
			lisp_cell_t *head = exp;
			if (is_nil(exp))
				DEBUG_RETURN(l->nil);
			for (exp = head; !is_nil(cdr(exp)); exp = cdr(exp)) {
				l->gc_stack_used = gc_stack_save;
				(void)eval(l, depth + 1, car(exp), env);
			}
			exp = car(exp);
			goto tail;
		}

		if(first == l->dowhile) {
			lisp_cell_t *wh = car(exp), *head = cdr(exp);
			while(!is_nil(eval(l, depth + 1, wh, env))) {
				l->gc_stack_used = gc_stack_save;
				for(exp = head; is_cons(exp); exp = cdr(exp)) 
					(void)eval(l, depth + 1, car(exp), env);
				if(!is_nil(exp))
					LISP_RECOVER(l, "%r\"while cannot eval dotted pairs\"%t\n '%S", head);
			}
			DEBUG_RETURN(l->nil);
		}

		if(first == l->macro) {
			/**@todo implement me*/
		}

		proc = eval(l, depth + 1, first, env);
		if (is_proc(proc) || is_subr(proc)) {	/*eval their args */
			vals = evlis(l, depth + 1, exp, env);
		} else if (is_fproc(proc)) {	/*f-expr do not eval their args */
			vals = cons(l, exp, l->nil);
		} else {
			LISP_RECOVER(l, "%r\"not a procedure\"%t\n '%S", first);
		}
		l->cur_depth = depth;	/*tucked away for function use */
		l->cur_env = env;	/*also tucked away */
		if (is_subr(proc)) {
			l->gc_stack_used = gc_stack_save;
			lisp_gc_add(l, proc);
			lisp_gc_add(l, vals);
			lisp_validate_cell(l, proc, vals, 1);
			DEBUG_RETURN((*get_subr(proc)) (l, vals));
		}
		if (is_proc(proc) || is_fproc(proc)) {
			if (get_length(get_proc_args(proc)) != get_length(vals))
				LISP_RECOVER(l, "%y'lambda%t\n '%S\n %y'expected%t\n '%S\n '%S", 
						get_func_docstring(proc), get_proc_args(proc), vals);
			if (get_length(get_proc_args(proc)))
				env = multiple_extend(l, (dynamic_on ? env : get_proc_env(proc)), get_proc_args(proc), vals);
			exp = cons(l, l->progn, get_proc_code(proc));
			goto tail;
		}
		LISP_RECOVER(l, "%r\"not a procedure\"%t\n '%S", first);
	case INVALID:
	default:
		LISP_HALT(l, "%r\"%s\"%t", "internal inconsistency: unknown type");
	}
	LISP_HALT(l, "%r\"%s\"%t", "internal inconsistency: reached the unreachable");
debug:
	lisp_log_debug(l, "%y'eval 'returned%t '%S", ret);
	return ret;
#undef DEBUG_RETURN
}

/**< evaluate a list*/
static lisp_cell_t *evlis(lisp_t * l, unsigned depth, lisp_cell_t * exps, lisp_cell_t * env)
{
	size_t i;
	lisp_cell_t *op, *head, *start = exps;
	assert(l && exps && env);
	if (is_nil(exps))
		return gsym_nil();
	op = car(exps);
	exps = cdr(exps);
	head = op = cons(l, eval(l, depth + 1, op, env), gsym_nil());
	for (i = 1; is_cons(exps); exps = cdr(exps), op = cdr(op), i++)
		set_cdr(op, cons(l, eval(l, depth + 1, car(exps), env), gsym_nil()));
	if(!is_nil(exps))
		LISP_RECOVER(l, "%r\"evlis cannot eval dotted pairs\"%t\n '%S", start);
	return head;
}

