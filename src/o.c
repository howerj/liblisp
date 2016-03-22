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

static lisp_cell_t *multiple_extend(lisp_t * l, lisp_cell_t *proc, lisp_cell_t * vals)
{
	assert(l && proc && vals);
	lisp_cell_t *env = dynamic_on ? l->cur_env : get_proc_env(proc);
       	lisp_cell_t *syms = get_proc_args(proc);
	for (; is_cons(syms); syms = cdr(syms), vals = cdr(vals))
		env = lisp_extend(l, env, car(syms), car(vals));
	if(!is_nil(syms))
		env = lisp_extend(l, env, syms, vals);
	/**@todo bind left over args */
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
	if(!exp || !env)
		return NULL;
	if (depth > MAX_RECURSION_DEPTH)
		LISP_RECOVER(l, "%y'recursion-depth-reached%t %d", 0);
	lisp_gc_add(l, exp);
	lisp_gc_add(l, env);
 tail:
	if(!exp || !env)
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
			LISP_RECOVER(l, "%r\"unbound symbol\"\n %y'%s%t", get_sym(exp));
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
			/*for (tmp = car(exp); is_cons(tmp); tmp = cdr(tmp));
			 *	if(!is_nil(tmp))
			 * 		mark_function_as_variadic()*/
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
		if (first == l->setq) {
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
		if (is_proc(proc) || is_subr(proc)) /*eval their args */
			vals = evlis(l, depth + 1, exp, env);
		else if (is_fproc(proc)) /*f-expr do not eval their args */
			vals = cons(l, exp, l->nil);
		else 
			LISP_RECOVER(l, "%r\"not a procedure\"%t\n '%S", first);
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
			/**@todo mark a function as variadic, and only then use
			 * greater than in get length check, this can be done
			 * by moving the checks into multiple_extend*/
			if (get_length(get_proc_args(proc)) > get_length(vals))
				LISP_RECOVER(l, "%y'arg-error%t\n %S\n '%S", proc, vals);
			if (get_length(get_proc_args(proc)))
				env = multiple_extend(l, proc, vals);
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

/** @file       hash.c
 *  @brief      A small hash library
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com **/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

/************************ small hash library **********************************/

static uint32_t hash_alg(hash_table_t * table, const char *s)
{
	return djb2(s, strlen(s)) % (table->len ? table->len : 1);
}

static hash_entry_t *hash_new_pair(const char *key, void *val)
{ /**@brief internal function to create a chained hash node**/
	hash_entry_t *np;
	if (!(np = calloc(1, sizeof(*np))))
		return NULL;
	np->key = (char *)key;
	np->val = val;
	if (!np->key || !np->val)
		return NULL;
	return np;
}

hash_table_t *hash_create(size_t len)
{
	return hash_create_auto_free(len, 0, 0);
}

hash_table_t *hash_create_auto_free(size_t len, unsigned free_key, unsigned free_value)
{
	hash_table_t *nt;
	if (!len)
		len++;
	if (!(nt = calloc(1, sizeof(*nt))))
		return NULL;
	if (!(nt->table = calloc(len, sizeof(*nt->table))))
		return free(nt), NULL;
	nt->len = len;
	nt->free_key = free_key;
	nt->free_value = free_value;
	return nt;
}

void hash_destroy(hash_table_t * h)
{
	size_t i;
	hash_entry_t *cur, *prev;
	if (!h)
		return;
	for (i = 0; i < h->len; i++)
		if (h->table[i]) {
			prev = NULL;
			for (cur = h->table[i]; cur; prev = cur, cur = cur->next) {
				if(h->free_key)
					free(cur->key);
				if(h->free_value)
					free(cur->val);
				free(prev);
			}
			if(h->free_key)
				free(cur->key);
			if(h->free_value)
				free(cur->val);
			free(prev);
		}
	free(h->table);
	free(h);
}

static int hash_grow(hash_table_t * ht)
{
	size_t i;
	hash_table_t *new;
	hash_entry_t *cur, *prev;
	memset(&new, 0, sizeof(new));
	if (!(new = hash_create(ht->len * 2)))
		goto fail;
	for (i = 0; i < ht->len; i++)
		if (ht->table[i])
			for (cur = ht->table[i]; cur; cur = cur->next)
				if (hash_insert(new, cur->key, cur->val) < 0)
					goto fail;
	for (i = 0; i < ht->len; i++)
		if (ht->table[i]) {
			prev = NULL;
			for (cur = ht->table[i]; cur; prev = cur, cur = cur->next)
				free(prev);
			free(prev);
		}
	free(ht->table);
	ht->table = new->table;
	ht->len = new->len;
	free(new);
	return 0;
 fail:	hash_destroy(new);
	return -1;
}

int hash_insert(hash_table_t * ht, const char *key, void *val)
{
	assert(ht && key && val);
	uint32_t hash = hash_alg(ht, key);
	hash_entry_t *cur, *newt, *last = NULL;

	if (hash_get_load_factor(ht) >= 0.75f)
		hash_grow(ht);

	cur = ht->table[hash];

	for (; cur && cur->key && strcmp(key, cur->key); cur = cur->next)
		last = cur;

	if (cur && cur->key && !strcmp(key, cur->key)) {
		ht->replacements++;
		cur->val = val;	/*replace */
	} else {
		if (!(newt = hash_new_pair(key, val)))
			return -1;
		ht->used++;
		if (cur == ht->table[hash]) {	/*collisions, insert head */
			ht->collisions++;
			newt->next = cur;
			ht->table[hash] = newt;
		} else if (!cur) {	/*free bin */
			last->next = newt;
		} else {	/*collision, insertions */
			ht->collisions++;
			newt->next = cur;
			last->next = newt;
		}
	}
	return 0;
}

void *hash_foreach(hash_table_t * h, hash_func func)
{
	assert(h && func);
	size_t i = 0;
	hash_entry_t *cur = NULL;
	void *ret;
	if (h->foreach) {
		i = h->foreach_index;
		cur = h->foreach_cur;
		goto resume;
	}
	h->foreach = 1;
	for (i = 0; i < h->len; i++)
		if (h->table[i])
			for (cur = h->table[i]; cur;) {
				if ((ret = (*func) (cur->key, cur->val))) {
					h->foreach_index = i;
					h->foreach_cur = cur;
					return ret;
				}
 resume:			cur = cur->next;
			}
	h->foreach = 0;
	return NULL;
}

void hash_reset_foreach(hash_table_t * h)
{
	h->foreach = 0;
}

static void *hprint(const char *key, void *val)
{
	assert(key);
	return printf("(\"%s\" %p)\n", key, val), NULL;
}

void hash_print(hash_table_t * h)
{
	assert(h);
	hash_foreach(h, hprint);
}

double hash_get_load_factor(hash_table_t * h)
{
	assert(h && h->len);
	return (double)h->used / h->len;
}

size_t hash_get_collision_count(hash_table_t * h)
{
	assert(h);
	return h->collisions;
}

size_t hash_get_replacements(hash_table_t * h)
{
	assert(h);
	return h->replacements;
}

size_t hash_get_number_of_bins(hash_table_t * h)
{
	assert(h);
	return h->len;
}

void *hash_lookup(hash_table_t * h, const char *key)
{
	uint32_t hash;
	hash_entry_t *cur;
	assert(h && key);

	hash = hash_alg(h, key);
	cur = h->table[hash];
	while (cur && cur->next && strcmp(cur->key, key))
		cur = cur->next;
	if (!cur || !cur->key || strcmp(cur->key, key))
		return NULL;
	else
		return cur->val;
}

/** @file       io.c
 *  @brief      An input/output port wrapper
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *  @todo       set error flags for strings, also refactor code
 **/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int io_is_in(io_t * i)
{
	assert(i);
	return (i->type == IO_FIN || i->type == IO_SIN);
}

int io_is_out(io_t * o)
{
	assert(o);
	return (o->type == IO_FOUT || o->type == IO_SOUT || o->type == IO_NULLOUT);
}

int io_is_file(io_t * f)
{
	assert(f);
	return (f->type == IO_FIN || f->type == IO_FOUT);
}

int io_is_string(io_t * s)
{
	assert(s);
	return (s->type == IO_SIN || s->type == IO_SOUT);
}

int io_is_null(io_t * n)
{
	assert(n);
	return n->type == IO_NULLOUT;
}

int io_getc(io_t * i)
{
	assert(i);
	int r;
	if (i->ungetc)
		return i->ungetc = 0, i->c;
	if (i->type == IO_FIN) {
		if ((r = fgetc(i->p.file)) == EOF)
			i->eof = 1;
		return r;
	}
	if (i->type == IO_SIN)
		return i->position < i->max ? i->p.str[i->position++] : EOF;
	FATAL("unknown or invalid IO type");
	return i->eof = 1, EOF;
}

char *io_get_string(io_t * x)
{
	assert(x && io_is_string(x));
	return x->p.str;
}

FILE *io_get_file(io_t * x)
{
	assert(x && io_is_file(x));
	return x->p.file;
}

int io_ungetc(char c, io_t * i)
{
	assert(i);
	if (i->ungetc)
		return i->eof = 1, EOF;
	i->c = c;
	i->ungetc = 1;
	return c;
}

int io_putc(char c, io_t * o)
{
	assert(o);
	int r;
	char *p;
	size_t maxt;
	if (o->type == IO_FOUT) {
		if ((r = fputc(c, o->p.file)) == EOF)
			o->eof = 1;
		return r;
	}
	if (o->type == IO_SOUT) {
		if (o->position >= (o->max - 1)) {	/*grow the "file" */
			maxt = (o->max + 1) * 2;
			if (maxt < o->position)	/*overflow */
				return o->eof = 1, EOF;
			o->max = maxt;
			if (!(p = realloc(o->p.str, maxt)))
				return o->eof = 1, EOF;
			memset(p + o->position, 0, maxt - o->position);
			o->p.str = p;
		}
		o->p.str[o->position++] = c;
		return c;
	}
	if (o->type == IO_NULLOUT)
		return c;
	FATAL("unknown or invalid IO type");
	return o->eof = 1, EOF;
}

int io_puts(const char *s, io_t * o)
{
	assert(s && o);
	int r;
	if (o->type == IO_FOUT) {
		if ((r = fputs(s, o->p.file)) == EOF)
			o->eof = 1;
		return r;
	}
	if (o->type == IO_SOUT) {
		/*this "grow" functionality should be moved into a function*/
		char *p;
		size_t len = strlen(s), newpos, maxt;
		if (o->position + len >= (o->max - 1)) {/*grow the "file" */
			maxt = (o->position + len) * 2;
			if (maxt < o->position)	/*overflow */
				return o->eof = 1, EOF;
			o->max = maxt;
			if (!(p = realloc(o->p.str, maxt)))
				return o->eof = 1, EOF;
			memset(p + o->position, 0, maxt - o->position);
			o->p.str = p;
		}
		newpos = o->position + len;
		if (newpos >= o->max)
			len = newpos - o->max;
		memmove(o->p.str + o->position, s, len);
		o->position = newpos;
		return len;
	}
	if (o->type == IO_NULLOUT)
		return (int)strlen(s);
	FATAL("unknown or invalid IO type");
	return EOF;
}

size_t io_read(char *ptr, size_t size, io_t *i)
{
	if(i->type == IO_FIN)
		return fread(ptr, 1, size, io_get_file(i));
	if(i->type == IO_SIN) {
		size_t copy = MIN(size, i->max - i->position);
		memcpy(ptr, i->p.str + i->position, copy);
		i->position += copy;
		return copy;
	}
	FATAL("unknown or invalid IO type");
	return 0;
}

size_t io_write(char *ptr, size_t size, io_t *o)
{ /**@todo test me, this function is untested*/
	if(o->type == IO_SOUT) {
		char *p;
		size_t newpos, maxt; 
		if (o->position + size >= (o->max - 1)) {/*grow the "file" */
			maxt = (o->position + size) * 2;
			if (maxt < o->position)	/*overflow */
				return o->eof = 1, EOF;
			o->max = maxt;
			if (!(p = realloc(o->p.str, maxt)))
				return o->eof = 1, EOF;
			memset(p + o->position, 0, maxt - o->position);
			o->p.str = p;
		}
		newpos = o->position + size;
		if (newpos >= o->max)
			size = newpos - o->max;
		memmove(o->p.str + o->position, ptr, size);
		o->position = newpos;
		return size;
	}
	if(o->type == IO_FOUT)
		return fwrite(ptr, 1, size, io_get_file(o));
	if(o->type == IO_NULLOUT)
		return size;
	FATAL("unknown or invalid IO type");
	return 0;
}

char *io_getdelim(io_t * i, int delim)
{
	assert(i);
	char *newbuf, *retbuf = NULL;
	size_t nchmax = 1, nchread = 0;
	int c;
	if (!(retbuf = calloc(1, 1)))
		return NULL;
	while ((c = io_getc(i)) != EOF) {
		if (nchread >= nchmax) {
			nchmax = nchread * 2;
			if (nchread >= nchmax)	/*overflow check */
				return free(retbuf), NULL;
			if (!(newbuf = realloc(retbuf, nchmax + 1)))
				return free(retbuf), NULL;
			retbuf = newbuf;
		}
		if (c == delim)
			break;
		retbuf[nchread++] = c;
	}
	if (!nchread && c == EOF)
		return free(retbuf), NULL;
	if (retbuf)
		retbuf[nchread] = '\0';
	return retbuf;
}

char *io_getline(io_t * i)
{
	assert(i);
	return io_getdelim(i, '\n');
}

int io_printd(intptr_t d, io_t * o)
{
	assert(o);
	if (o->type == IO_FOUT)
		return fprintf(o->p.file, "%" PRIiPTR, d);
	if (o->type == IO_SOUT) {
		char dstr[64] = "";
		sprintf(dstr, "%" SCNiPTR, d);
		return io_puts(dstr, o);
	}
	return EOF;
}

int io_printflt(double f, io_t * o)
{
	assert(o);
	if (o->type == IO_FOUT)
		return fprintf(o->p.file, "%e", f);
	if (o->type == IO_SOUT) {
		/**@note if using %f the numbers can printed can be very large (~512 characters long) */
		char dstr[32] = "";
		sprintf(dstr, "%e", f);
		return io_puts(dstr, o);
	}
	return EOF;
}

io_t *io_sin(const char *sin, size_t len)
{
	io_t *i;
	if (!sin || !(i = calloc(1, sizeof(*i))))
		return NULL;
	if (!(i->p.str = calloc(len, 1)))
		return NULL;
	memcpy(i->p.str, sin, len);
	i->type = IO_SIN;
	i->max = len;
	return i;
}

io_t *io_fin(FILE * fin)
{
	io_t *i;
	if (!fin || !(i = calloc(1, sizeof(*i))))
		return NULL;
	i->p.file = fin;
	i->type = IO_FIN;
	return i;
}

io_t *io_sout(size_t len)
{
	io_t *o;
	char *sout;
	len = len == 0 ? 1 : len;
	if (!(sout = calloc(len, 1)) || !(o = calloc(1, sizeof(*o))))
		return NULL;
	o->p.str = sout;
	o->type = IO_SOUT;
	o->max = len;
	return o;
}

io_t *io_fout(FILE * fout)
{
	io_t *o;
	if (!fout || !(o = calloc(1, sizeof(*o))))
		return NULL;
	o->p.file = fout;
	o->type = IO_FOUT;
	return o;
}

io_t *io_nout(void)
{
	io_t *o;
	if (!(o = calloc(1, sizeof(*o))))
		return NULL;
	o->type = IO_NULLOUT;
	return o;
}

int io_close(io_t * c)
{
	int ret = 0;
	if (!c)
		return -1;
	if (c->type == IO_FIN || c->type == IO_FOUT)
		if (c->p.file != stdin && c->p.file != stdout && c->p.file != stderr)
			ret = fclose(c->p.file);
	if (c->type == IO_SIN)
		free(c->p.str);
	free(c);
	return ret;
}

int io_eof(io_t * f)
{
	assert(f);
	if (f->type == IO_FIN || f->type == IO_FOUT)
		f->eof = feof(f->p.file) ? 1 : 0;
	return f->eof;
}

int io_flush(io_t * f)
{
	assert(f);
	if (f->type == IO_FIN || f->type == IO_FOUT)
		return fflush(f->p.file);
	return 0;
}

long io_tell(io_t * f)
{
	assert(f);
	if (f->type == IO_FIN || f->type == IO_FOUT)
		return ftell(f->p.file);
	if (f->type == IO_SIN || f->type == IO_SOUT)
		return f->position;
	return -1;
}

int io_seek(io_t * f, long offset, int origin)
{
	assert(f);
	if (f->type == IO_FIN || f->type == IO_FOUT)
		return fseek(f->p.file, offset, origin);
	if (f->type == IO_SIN || f->type == IO_SOUT) {
		if (!f->max)
			return -1;
		switch (origin) {
		case SEEK_SET:
			f->position = offset;
			break;
		case SEEK_CUR:
			f->position += offset;
			break;
		case SEEK_END:
			f->position = f->max - offset;
			break;
		default:
			return -1;
		}
		return f->position = MIN(f->position, f->max);
	}
	return -1;
}

int io_error(io_t * f)
{
	assert(f);
	if (f->type == IO_FIN || f->type == IO_FOUT)
		return ferror(f->p.file);
	return 0;
}

void io_color(io_t * out, int color_on)
{
	assert(out);
	out->color = color_on;
}

void io_pretty(io_t * out, int pretty_on)
{
	assert(out);
	out->pretty = pretty_on;
}

/** @file       lisp.c
 *  @brief      A minimal lisp interpreter and utility library, interface
 *              functions.
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
#include <limits.h>

void lisp_throw(lisp_t * l, int ret)
{
	if (l && !l->errors_halt && l->recover_init)
		longjmp(l->recover, ret);
	else
		exit(ret);
}

lisp_cell_t *lisp_environment(lisp_t *l)
{
	return l->top_env;
}

int lisp_add_module_subroutines(lisp_t *l, const lisp_module_subroutines_t *ms, size_t len)
{
	for(size_t i = 0; ms[i].name && (!len || i < len); i++)
		if(!lisp_add_subr(l, ms[i].name, ms[i].p, ms[i].validate, ms[i].docstring))
			return -1;
	return 0;
}

char *lisp_strdup(lisp_t *l, const char *s)
{
	assert(l && s);
	char *r = lstrdup(s);
	if(!r)
		LISP_HALT(l, "\"%s\"", "out of memory");
	return r;
}

lisp_cell_t *lisp_add_subr(lisp_t * l, const char *name, lisp_subr_func func, const char *fmt, const char *doc)
{
	assert(l && name && func);	/*fmt and doc are optional */
	return lisp_extend_top(l, lisp_intern(l, lisp_strdup(l, name)), mk_subr(l, func, fmt, doc));
}

lisp_cell_t *lisp_get_all_symbols(lisp_t * l)
{
	assert(l);
	return l->all_symbols;
}

lisp_cell_t *lisp_add_cell(lisp_t * l, const char *sym, lisp_cell_t * val)
{
	assert(l && sym && val);
	return lisp_extend_top(l, lisp_intern(l, lisp_strdup(l, sym)), val);
}

void lisp_destroy(lisp_t * l)
{
	if (!l)
		return;
	free(l->buf);
	l->gc_off = 0;
	if (l->gc_stack)
		lisp_gc_sweep_only(l), free(l->gc_stack);
	if (lisp_get_logging(l))
		io_close(lisp_get_logging(l));
	if (lisp_get_output(l))
		io_close(lisp_get_output(l));
	if (lisp_get_input(l))
		io_close(lisp_get_input(l));
	free(l->input);
	free(l->output);
	free(l->logging);
	free(l);
}

lisp_cell_t *lisp_read(lisp_t * l, io_t * i)
{
	assert(l && i);
	lisp_cell_t *ret;
	int restore_used, r;
	jmp_buf restore;
	if (l->recover_init) {
		memcpy(restore, l->recover, sizeof(jmp_buf));
		restore_used = 1;
	}
	if ((r = setjmp(l->recover))) {
		LISP_RECOVER_RESTORE(restore_used, l, restore);
		return r > 0 ? l->error : NULL;
	}
	l->recover_init = 1;
	ret = reader(l, i);
	LISP_RECOVER_RESTORE(restore_used, l, restore);
	return ret;
}

int lisp_print(lisp_t * l, lisp_cell_t * ob)
{
	assert(l && ob);
	int ret = printer(l, lisp_get_output(l), ob, 0);
	io_putc('\n', lisp_get_output(l));
	return ret;
}

lisp_cell_t *lisp_eval(lisp_t * l, lisp_cell_t * exp)
{
	assert(l && exp);
	lisp_cell_t *ret;
	int restore_used, r;
	jmp_buf restore;
	if (l->recover_init) {
		memcpy(restore, l->recover, sizeof(jmp_buf));
		restore_used = 1;
	}
	if ((r = setjmp(l->recover))) {
		LISP_RECOVER_RESTORE(restore_used, l, restore);
		return r > 0 ? l->error : NULL;
	}
	l->recover_init = 1;
	ret = eval(l, 0, exp, l->top_env);
	LISP_RECOVER_RESTORE(restore_used, l, restore);
	return ret;
}

/**@bug the entire string should be evaluated, not just the first expression */
lisp_cell_t *lisp_eval_string(lisp_t * l, const char *evalme)
{
	assert(l && evalme);
	io_t *in = NULL;
	lisp_cell_t *ret;
	volatile int restore_used = 0, r;
	jmp_buf restore;
	if (!(in = io_sin(evalme, strlen(evalme))))
		return NULL;
	if (l->recover_init) {
		memcpy(restore, l->recover, sizeof(jmp_buf));
		restore_used = 1;
	}
	if ((r = setjmp(l->recover))) {
		io_close(in);
		LISP_RECOVER_RESTORE(restore_used, l, restore);
		return r > 0 ? l->error : NULL;
	}
	l->recover_init = 1;
	ret = eval(l, 0, reader(l, in), l->top_env);
	io_close(in);
	LISP_RECOVER_RESTORE(restore_used, l, restore);
	return ret;
}

int lisp_log_error(lisp_t *l, char *fmt, ...) 
{
	int ret = 0;
	if(lisp_get_log_level(l) >= LISP_LOG_LEVEL_ERROR) {
		va_list ap;
		io_t *e = lisp_get_logging(l);
		va_start(ap, fmt);
		lisp_printf(l, e, 0, "(%rerror%t ");
		ret = lisp_vprintf(l, lisp_get_logging(l), 0, fmt, ap);
		lisp_printf(l, e, 0, ")%t\n");
		va_end(ap);
	}
	return ret;
}

int lisp_log_note(lisp_t *l, char *fmt, ...) 
{
	int ret = 0;
	if(lisp_get_log_level(l) >= LISP_LOG_LEVEL_NOTE) {
        	va_list ap;
		io_t *e = lisp_get_logging(l);
        	va_start(ap, fmt);
		lisp_printf(l, e, 0, "(%ynote%t ");
		ret = lisp_vprintf(l, e, 0, fmt, ap);
		lisp_printf(l, e, 0, ")%t\n");
		va_end(ap);
	}
	return ret;
}

int lisp_log_debug(lisp_t *l, char *fmt, ...) 
{
	int ret = 0;
	if(lisp_get_log_level(l) >= LISP_LOG_LEVEL_DEBUG) {
		va_list ap;
		io_t *e = lisp_get_logging(l);
		va_start(ap, fmt);
		lisp_printf(l, e, 0, "(%mdebug%t ");
		ret = lisp_vprintf(l, e, 0, fmt, ap);
		lisp_printf(l, e, 0, ")%t\n");
		va_end(ap);
	}
	return ret;
}

int lisp_set_input(lisp_t * l, io_t * in)
{
	assert(l);
	l->input->p[0].v = in;
	if (!in || !io_is_in(in))
		return -1;
	return 0;
}

int lisp_set_output(lisp_t * l, io_t * out)
{
	assert(l);
	l->output->p[0].v = out;
	if (!out || !io_is_out(out))
		return -1;
	return 0;
}

int lisp_set_logging(lisp_t * l, io_t * logging)
{
	assert(l);
	l->logging->p[0].v = logging;
	if (!logging || !io_is_out(logging))
		return -1;
	return 0;
}

void lisp_set_line_editor(lisp_t * l, lisp_editor_func ed)
{
	assert(l);
	l->editor = ed;
}

void lisp_set_signal(lisp_t * l, int sig)
{
	assert(l);
	l->sig = sig;
}

io_t *lisp_get_input(lisp_t * l)
{
	assert(l);
	return get_io(l->input);
}

io_t *lisp_get_output(lisp_t * l)
{
	assert(l);
	return get_io(l->output);
}

io_t *lisp_get_logging(lisp_t * l)
{
	assert(l);
	return get_io(l->logging);
}

void lisp_set_log_level(lisp_t *l, lisp_log_level level)
{
	assert(l && level < LISP_LOG_LEVEL_LAST_INVALID);
	l->log_level = level;
}

lisp_log_level lisp_get_log_level(lisp_t *l)
{
	assert(l);
	return l->log_level;
}

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

/** @file       print.c
 *  @brief      print out S-expressions
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com 
 *  @warning    Colorization supported with ANSI escape codes only **/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

char *lisp_serialize(lisp_t *l, lisp_cell_t *x)
{
	assert(l && x);
	char *rs = NULL;
	io_t *s = io_sout(2);
	if(!s)
		goto fail;
	if(printer(l, s, x, 0) < 0)
		goto fail;
	rs = io_get_string(s);
	io_close(s); /*this does not free the string it contains*/
	return rs;
fail:
	if(s) {
		free(io_get_string(s));
		io_close(s);
	}
	return NULL;
}

static int print_escaped_string(lisp_t * l, io_t * o, unsigned depth, char *s)
{
	int ret, m = 0;
	char c;
	assert(l && o && s);
	if((ret = lisp_printf(l, o, depth, "%r\"")) < 0)
		return -1;
	while ((c = *s++)) {
		ret += m;
		switch (c) {
		case '\\':
			if((m = lisp_printf(l, o, depth, "%m\\\\%r")) < 0)
				return -1;
			continue;
		case '\n':
			if((m = lisp_printf(l, o, depth, "%m\\n%r")) < 0)
				return -1;
			continue;
		case '\t':
			if((m = lisp_printf(l, o, depth, "%m\\t%r")) < 0)
				return -1;
			continue;
		case '\r':
			if((m = lisp_printf(l, o, depth, "%m\\r%r")) < 0)
				return -1;
			continue;
		case '"':
			if((m = lisp_printf(l, o, depth, "%m\\\"%r")) < 0)
				return -1;
			continue;
		default:
			break;
		}
		if (!isprint(c)) {
			char num[5] = "\\";
			sprintf(num + 1, "%03o", ((unsigned)c) & 0xFF);
			assert(!num[4]);
			if((m = lisp_printf(l, o, depth, "%m%s%r", num)) < 0)
				return -1;
			continue;
		}
		if((m = io_putc(c, o)) < 0)
			return -1;
	}
	if((m = io_putc('"', o)) < 0)
		return -1;
	return ret + m;
}

int lisp_printf(lisp_t *l, io_t *o, unsigned depth, char *fmt, ...) 
{
	int ret;
	va_list ap;
	va_start(ap, fmt);
	ret = lisp_vprintf(l, o, depth, fmt, ap);
	va_end(ap);
	return ret;
}

static int print_hash(lisp_t *l, io_t *o, unsigned depth, hash_table_t *ht)
{
	int ret, m, n;
	size_t i;
	hash_entry_t *cur;
	if((ret = lisp_printf(l, o, depth, "{")) < 0)
		return -1;
	for(i = 0; i < ht->len; i++)
		if(ht->table[i])
		/**@warning messy hash stuff*/
		for(cur = ht->table[i]; cur; cur = cur->next) {
			io_putc(' ', o);
			if(is_cons(cur->val) && is_sym(car(cur->val)))
				m = lisp_printf(l, o, depth, "%S", car(cur->val));
			else
				m = print_escaped_string(l, o, depth, cur->key);

			if(is_cons(cur->val)) 
				n = lisp_printf(l, o, depth, "%t %S", cdr(cur->val));
			else
				n = lisp_printf(l, o, depth, "%t %S", cur->val);
			if(m < 0 || n < 0)
				return -1;
			ret += m + n;
		}
	if((m = io_puts(" }", o)) < 0)
		return -1;
	return ret + m;
}

int lisp_vprintf(lisp_t *l, io_t *o, unsigned depth, char *fmt, va_list ap)
{
	intptr_t d;
	unsigned dep;
	double flt;
	char c, f, *s;
	int ret = 0;
	hash_table_t *ht;
	lisp_cell_t *ob;
	while(*fmt) {
		if(ret == EOF) goto finish;
		if('%' == (f = *fmt++)) {
			switch (f = *fmt++) {
			case '\0': 
				goto finish;
			case '%':  
				ret = io_putc('%', o);
				break;
			case '@':  
				f = *fmt++;
				if(!f) goto finish;
				dep = depth;
				while(dep--)
					ret = io_putc(f, o); 
				break;
			case 'c':  
				c = va_arg(ap, int);
				ret = io_putc(c, o);
				break;
			case 's':  
				s = va_arg(ap, char*);
				ret = io_puts(s, o);
				break;
			case 'd':  
				d = va_arg(ap, intptr_t);
				ret = io_printd(d, o); 
				break;
			case 'f':  
				flt = va_arg(ap, double);
				ret = io_printflt(flt, o);
				break;
			case 'S':  
				ob = va_arg(ap, lisp_cell_t *);
				ret =  printer(l, o, ob, depth);
				break;
			case 'H':  
				ht = va_arg(ap, hash_table_t *);
				ret = print_hash(l, o, depth, ht);
				break;
			default:   
				if(o->color) {
					char *color = "";
					switch(f) {
					case 't': color = "\x1b[0m";  break; /*reset*/
					case 'B': color = "\x1b[1m";  break; /*bold text*/
					case 'v': color = "\x1b[7m";  break; /*reverse video*/
					case 'k': color = "\x1b[30m"; break; /*black*/
					case 'r': color = "\x1b[31m"; break; /*red*/
					case 'g': color = "\x1b[32m"; break; /*green*/ 
					case 'y': color = "\x1b[33m"; break; /*yellow*/
					case 'b': color = "\x1b[34m"; break; /*blue*/
					case 'm': color = "\x1b[35m"; break; /*magenta*/
					case 'a': color = "\x1b[36m"; break; /*cyan*/
					case 'w': color = "\x1b[37m"; break; /*white*/
					default: /*return -1:*/ break;
					}
					ret = io_puts(color, o);
				}
				break;
			}
		} else {
			ret = io_putc(f, o);
		}
	}
finish: 
	return ret;
}

int printer(lisp_t *l, io_t *o, lisp_cell_t *op, unsigned depth) 
{ 
	lisp_cell_t *tmp;
	if(!op) 
		return EOF;
	if(l && depth > MAX_RECURSION_DEPTH) {
		lisp_log_error(l, "%r'print-depth-exceeded %d%t", (intptr_t) depth);
		return -1;
	}
	switch(op->type) {
	case INTEGER: 
		lisp_printf(l, o, depth, "%m%d", get_int(op));   
		break; 
	case FLOAT:   
		lisp_printf(l, o, depth, "%m%f", get_float(op)); 
		break; 
	case CONS:    
		if(depth && o->pretty) lisp_printf(l, o, depth, "\n%@ ");
			io_putc('(', o);
			for(;;) {
				printer(l, o, car(op), depth + 1);
				if(is_nil(cdr(op))) {
					io_putc(')', o);
					break;
				}
				op = cdr(op);
				if(!is_cons(op)) {
					lisp_printf(l, o, depth, " . %S)", op);
					break;
				}
				io_putc(' ', o);
			}
			break;
	case SYMBOL:
		if(is_nil(op)) lisp_printf(l, o, depth, "%rnil");
		else           lisp_printf(l, o, depth, "%y%s", get_sym(op));
		break;
	case STRING:
		print_escaped_string(l, o, depth, get_str(op));       
		break;
	case SUBR:
		lisp_printf(l, o, depth, "%B<subroutine:%d>", get_int(op)); 
		break;
	case PROC: case FPROC: 
		lisp_printf(l, o, depth+1, 
			is_proc(op) ? "(%ylambda%t %S %S " :
				      "(%yflambda%t %S %S ", 
					get_func_docstring(op), get_proc_args(op));
		for(tmp = get_proc_code(op); !is_nil(tmp); tmp = cdr(tmp)) {
			printer(l, o, car(tmp), depth+1);
			if(!is_nil(cdr(tmp)))
				io_putc(' ', o);
		}
		io_putc(')', o);
		break;
	case HASH:
		lisp_printf(l, o, depth, "%H", get_hash(op)); 
		break;
	case IO:	
		lisp_printf(l, o, depth, "%B<io:%s:%d>",  
			op->close? "closed" : 
				(is_in(op)? "in" : "out"), get_int(op)); 
		break;
	case USERDEF:
		if(l && l->ufuncs[get_user_type(op)].print)
			(l->ufuncs[get_user_type(op)].print)(o, depth, op);
		else lisp_printf(l, o, depth, "<user:%d:%d>",
			get_user_type(op), get_int(op)); break;
	case INVALID: 
	default:      
		FATAL("internal inconsistency");
	}
	return lisp_printf(l, o, depth, "%t") == EOF ? EOF : 0;
}

/** @file       read.c
 *  @brief      An S-Expression parser for liblisp
 *  @author     Richard Howe (2015, 2016)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com 
 *  
 *  An S-Expression parser, it takes it's input from a generic input
 *  port that can be set up to read from a string or a file. 
 *  @todo quasiquote, unquote, unquote-splicing, compose, negate, and
 *        runs of car and cdr.
 *  @bug '('a . 'b)
 **/
#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* These are options that control what gets parsed */
static const int parse_strings = 1,	/*parse strings? e.g. "Hello" */
    parse_floats = 1,		/*parse floating point numbers? e.g. 1.3e4 */
    parse_ints   = 1,		/*parse integers? e.g. 3 */
    parse_hashes = 1, 		/*parse hashes? e.g. { a b c (1 2 3) } */
    parse_sugar  = 1,           /*parse syntax sugar eg a.b <=> (a b) */
    parse_dotted = 1;		/*parse dotted pairs? e.g. (a . b) */

/**@brief process a comment from I/O stream**/
static int comment(io_t * i)
{
	int c;
	while (((c = io_getc(i)) > 0) && (c != '\n')) ;
	return c;
}

/**@brief add a char to the token buffer*/
static void add_char(lisp_t * l, char ch)
{
	assert(l);
	char *tmp;
	if (l->buf_used > l->buf_allocated - 1) {
		l->buf_allocated = l->buf_used * 2;
		if (l->buf_allocated < l->buf_used)
			LISP_HALT(l, "%s", "overflow in allocator size variable");
		if (!(tmp = realloc(l->buf, l->buf_allocated)))
			LISP_HALT(l, "%s", "out of memory");
		l->buf = tmp;
	}
	l->buf[l->buf_used++] = ch;
}

/**@brief allocate a new token */
static char *new_token(lisp_t * l)
{
	assert(l);
	l->buf[l->buf_used++] = '\0';
	return lisp_strdup(l, l->buf);
}

/**@brief push back a single token */
static void unget_token(lisp_t * l, char *token)
{
	assert(l && token);
	l->token = token;
	l->ungettok = 1;
}

static const char lex[] = "(){}\'\""; /**@todo add '`', ','*/
static char *lexer(lisp_t * l, io_t * i)
{
	assert(l && i);
	int ch, end = 0;
	l->buf_used = 0;
	if (l->ungettok)
		return l->ungettok = 0, l->token;
	do {
		if ((ch = io_getc(i)) == EOF)
			return NULL;
		if (ch == '#' || ch == ';') {
			comment(i);
			continue;
		}		/*ugly*/
	} while (isspace(ch) || ch == '#' || ch == ';');
	add_char(l, ch);
	/**@bug if parse_hashes is off, "{}" gets processed as two tokens*/
	if (strchr(lex, ch))
		return new_token(l);
	for (;;) {
		if ((ch = io_getc(i)) == EOF)
			end = 1;
		if (ch == '#' || ch == ';') {
			comment(i);
			continue;
		}		/*ugly */
		if (strchr(lex, ch) || isspace(ch)) {
			io_ungetc(ch, i);
			return new_token(l);
		}
		if (end)
			return new_token(l);
		add_char(l, ch);
	}
}

/**@brief handle parsing a string*/
static char *read_string(lisp_t * l, io_t * i)
{
	assert(l && i);					   
	int ch;
	char num[4] = { 0, 0, 0, 0 };
	l->buf_used = 0;
	for (;;) {
		if ((ch = io_getc(i)) == EOF)
			return NULL;
		if (ch == '\\') {
			ch = io_getc(i);
			switch (ch) {
			case '\\':
				add_char(l, '\\');
				continue;
			case 'n':
				add_char(l, '\n');
				continue;
			case 't':
				add_char(l, '\t');
				continue;
			case 'r':
				add_char(l, '\r');
				continue;
			case '"':
				add_char(l, '"');
				continue;
			case '0':
			case '1':
			case '2':
			case '3':
				num[0] = ch;
				if(io_read(num+1, 2, i) != 2)
				       goto fail;
				if (num[strspn(num, "01234567")])
					goto fail;
				ch = (char)strtol(num, NULL, 8);
				if (!ch) /*currently cannot handle NUL in strings*/
					goto fail;
				add_char(l, ch);
				continue;
 fail:				LISP_RECOVER(l, "%y'invalid-escape-literal\n %r\"%s\"%t", num);
			case EOF:
				return NULL;
			default:
				LISP_RECOVER(l, "%y'invalid-escape-char\n %r\"%c\"%t", ch);
			}
		}
		if (ch == '"')
			return new_token(l);
		add_char(l, ch);
	}
	return NULL;
}

static int keyval(lisp_t * l, io_t * i, hash_table_t *ht, char *key)
{
	lisp_cell_t *val;
	if(!(val = reader(l, i)))
		return -1;
	if(hash_insert(ht, key, cons(l, mk_str(l, key), val)) < 0)
		return -1;
	return 0;
}

static lisp_cell_t *read_hash(lisp_t * l, io_t * i)
{
	hash_table_t *ht;
	char *token = NULL; 
	if (!(ht = hash_create(SMALL_DEFAULT_LEN))) /**@bug leaks memory on error*/
		LISP_HALT(l, "%s", "out of memory");
	for(;;) {
		token = lexer(l, i);
		if(!token)
			goto fail;
		switch(token[0]) {
		case '}': 
			free(token);
			return mk_hash(l, ht);
		case '(': 
		case ')': 
		case '{': 
		case '\'':
		case '.': 
			goto fail;
		case '"': 
		{
			char *key;
			free(token);
			token = NULL;
			if(!(key = read_string(l, i)))
				goto fail;
			if(keyval(l, i, ht, key) < 0)
				goto fail;
			continue;
		}
		default:
			if(parse_ints && is_number(token)) {
				goto fail;
			} else if(parse_floats && is_fnumber(token)) {
				goto fail;
			}

			if(keyval(l, i, ht, new_token(l)) < 0)
				goto fail;
			free(token);
			continue;
		}
	}
fail:
	if(token)
		LISP_RECOVER(l, "%y'invalid-hash-key%t %r\"%s\"%t", token);
	hash_destroy(ht);
	free(token);
	return NULL;
}

static lisp_cell_t *new_sym(lisp_t *l, const char *token, size_t end)
{
	lisp_cell_t *ret;
	assert(l && token && end);
	if((parse_ints && is_number(token)) || (parse_floats && is_fnumber(token)))
		LISP_RECOVER(l, "%r\"unexpected integer or float\"\n %m%s%t", token);
	char *tnew = calloc(end+1, 1);
	if(!tnew)
		LISP_HALT(l, "%s", "out of memory");
	memcpy(tnew, token, end);
	ret = lisp_intern(l, tnew);
	if(get_sym(ret) != tnew)
		free(tnew);
	return ret;
}

static const char symbol_splitters[] = ".!"; /**@note '~' (negate) and ':' (compose) go here, when implemented*/
static lisp_cell_t *process_symbol(lisp_t *l, const char *token)
{
	size_t i;
	assert(l && token);
	if(!parse_sugar)
		goto nosugar;
	if(!token[0])
		goto fail;

	if(strchr(symbol_splitters, token[0]))
		LISP_RECOVER(l, "%r\"invalid prefix\"\n \"%s\"%t", token);

	i = strcspn(token, symbol_splitters);
	switch(token[i]) {
	case '.': /* a.b <=> (a b) */
		if(!token[i+1])
			goto fail;
		return mk_list(l, new_sym(l, token, i), process_symbol(l, token+i+1), NULL);
	case '!': /* a!b <=> (a 'b) */
		if(!token[i+1])
			goto fail;
		return mk_list(l, new_sym(l, token, i), mk_list(l, l->quote, process_symbol(l, token+i+1), NULL), NULL);
	default:
		break;
	}
nosugar:
	return new_sym(l, token, strlen(token));
fail:
	LISP_RECOVER(l, "%r\"invalid symbol/expected more\"\n \"%s\"%t", token);
	return NULL;
}

static lisp_cell_t *read_list(lisp_t * l, io_t * i);
lisp_cell_t *reader(lisp_t * l, io_t * i)
{
	assert(l && i);
	char *token = lexer(l, i), *fltend = NULL;
	double flt;
	lisp_cell_t *ret;
	if (!token)
		return NULL;
	switch (token[0]) {
	case '(':
		free(token);
		return read_list(l, i);
	case ')':
		free(token);
		LISP_RECOVER(l, "%r\"unmatched %s\"%t", "')'");
	case '{':
		if(!parse_hashes)
			goto nohash;
		free(token);
		return read_hash(l, i);
	case '}':
		if(!parse_hashes)
			goto nohash;
		free(token);
		LISP_RECOVER(l, "%r\"unmatched %s\"%t", "'}'");
	case '"':
	{
		char *s;
		if (!parse_strings)
			goto nostring;
		free(token);
		if(!(s = read_string(l, i)))
			return NULL;
		return mk_str(l, s);
	}
	case '\'':
		free(token);
		if(!(ret = reader(l, i)))
			return NULL;
		return mk_list(l, l->quote, ret, NULL);
/*	case '`': // TODO
		free(token);
		if(!(ret = reader(l, i)))
			return NULL;
		return mk_list(l, l->quasiquote, ret, NULL);
	case ',':
		free(token);
		if(!(ret = reader(l, i)))
			return NULL;
		return mk_list(l, l->unquote, ret, NULL);*/
	default:
		if (parse_ints && is_number(token)) {
			ret = mk_int(l, strtol(token, NULL, 0));
			free(token);
			return ret;
		}
		if (parse_floats && is_fnumber(token)) {
			flt = strtod(token, &fltend);
			if (!fltend[0]) {
				free(token);
				return mk_float(l, flt);
			}
		}
 nostring:
 nohash:
		ret = process_symbol(l, token);
		free(token);
		return ret;
	}
	return gsym_nil();
}

/**@brief read in a list*/
static lisp_cell_t *read_list(lisp_t * l, io_t * i)
{
	assert(l && i);
	char *token = lexer(l, i), *stok;
	lisp_cell_t *a, *b;
	if (!token) 
		return NULL;
	switch (token[0]) {
	case ')':
		free(token);
		return gsym_nil();
	case '}':
		free(token);
		return gsym_nil();
	case '.':
		if (!parse_dotted)
			goto nodots;
		if (!(a = reader(l, i)))
			return NULL;
		if (!(stok = lexer(l, i)))
			return NULL;
		if (strcmp(stok, ")")) {
			free(stok);
			LISP_RECOVER(l, "%y'invalid-cons%t %r\"%s\"%t", "unexpected right parenthesis");
		}
		free(token);
		free(stok);
		return a;
	default:
		break;
	}
 nodots:
	unget_token(l, token);
	if (!(a = reader(l, i)))
		return NULL;	/* force evaluation order */
	if (!(b = read_list(l, i)))
		return NULL;
	return cons(l, a, b);
}

/** @file       repl.c
 *  @brief      An example REPL for the liblisp interpreter
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com **/

#include "liblisp.h"
#include "private.h"
#include <stdlib.h>
#include <string.h>
#include <locale.h>

/**** The need putting here by the build system / version control system ****/
#ifndef VERSION
#define VERSION unknown    /**< Version of the interpreter*/
#endif
#ifndef VCS_COMMIT
#define VCS_COMMIT unknown /**< Version control commit of this interpreter*/
#endif
#ifndef VCS_ORIGIN
#define VCS_ORIGIN unknown /**< Version control repository origin*/
#endif
/****************************************************************************/

static const char *usage = /**< command line options for example interpreter*/
    "(-[hcpvVEHL])* (-[i\\-] file)* (-e string)* (-o file)* file* -";

static const char *help =
"The liblisp library and interpreter. For more information on usage\n\
consult the man pages 'lisp' and 'liblisp'. Alternatively, consult:\n\
\n\
	https://github.com/howerj/liblisp\n\
	http://work.anapnea.net/html/html/projects.html\n\
";

static unsigned lisp_verbosity = LISP_LOG_LEVEL_ERROR;

enum { OPTS_ERROR = -1,      /**< there's been an error processing the options*/
	OPTS_SWITCH,	     /**< current argument was a valid flag*/
	OPTS_IN_FILE,	     /**< current argument is file input to eval*/
	OPTS_IN_FILE_NEXT_ARG, /**< process the next argument as file input*/
	OPTS_OUT_FILE,	     /**< next argument is an output file*/
	OPTS_IN_STRING,	     /**< next argument is a string to eval*/
	OPTS_IN_STDIN,	     /**< read input from stdin*/
}; /**< getoptions enum*/

static int getoptions(lisp_t * l, char *arg, char *arg_0)
{ /**@brief simple parser for command line options**/
	int c;
	if ('-' != *arg++)
		return OPTS_IN_FILE;
	if (!arg[0])
		return OPTS_IN_STDIN;
	while ((c = *arg++))
		switch (c) {
		case 'i':
		case '-':
			return OPTS_IN_FILE_NEXT_ARG;
		case 'h':
			printf("usage %s %s\n\n", arg_0, usage);
			puts(help);
			exit(0);
			break;
		case 'c':
			lisp_log_note(l, "'color-on");
			l->color_on = 1;
			break;	/*colorize output */
		case 'L': 
			lisp_log_note(l, "'local 'default");
			if(!setlocale(LC_ALL, ""))
				FATAL("failed to default locale");
			break;
		case 'p':
			lisp_log_note(l, "'prompt-on");
			l->prompt_on = 1;
			break;	/*turn standard prompt when reading stdin */
		case 'E':
			lisp_log_note(l, "'line-editor-on");
			l->editor_on = 1;
			break;	/*turn line editor on when reading stdin */
		case 'H':
			lisp_log_note(l, "'halt-on-error");
			l->errors_halt = 1;
			break;
		case 'v':
			lisp_verbosity++;
			if(lisp_verbosity < LISP_LOG_LEVEL_LAST_INVALID)
				lisp_set_log_level(l, lisp_verbosity);
			else
				lisp_log_note(l, "'verbosity \"already set to maximum\"");
			break;
		case 'V':
			puts("program: liblisp");
			puts("version: " XSTRINGIFY(VERSION));
			puts("commit:  " XSTRINGIFY(VCS_COMMIT));
			puts("origin:  " XSTRINGIFY(VCS_ORIGIN));
			exit(0);
			break;
		case 'e':
			return OPTS_IN_STRING;
		case 'o':
			return OPTS_OUT_FILE;
		default:
			fprintf(stderr, "unknown option '%c'\n", c);
			fprintf(stderr, "usage %s %s\n", arg_0, usage);
			return OPTS_ERROR;
		}
	return OPTS_SWITCH;	/*this argument was a valid flag, nothing more */
}

int lisp_repl(lisp_t * l, char *prompt, int editor_on)
{
	lisp_cell_t *ret;
	io_t *ofp, *efp;
	char *line = NULL;
	int r = 0;
	ofp = lisp_get_output(l);
	efp = lisp_get_logging(l);
	ofp->pretty = efp->pretty = 1;
	ofp->color = efp->color = l->color_on;
	if ((r = setjmp(l->recover)) < 0) {	/*catch errors and "sig" */
		l->recover_init = 0;
		return r;
	}
	l->recover_init = 1;
	if (editor_on && l->editor) {	/*handle line editing functionality */
		while ((line = l->editor(prompt))) {
			lisp_cell_t *prn;
			if (!line[strspn(line, " \t\r\n")]) {
				free(line);
				continue;
			}
			if (!(prn = lisp_eval_string(l, line))) {
				free(line);
				LISP_RECOVER(l, "\"%s\"", "invalid or incomplete line");
			}
			lisp_print(l, prn);
			free(line);
			line = NULL;
		}
	} else {		/*read from input with no special handling, or a file */
		for (;;) {
			/**@bug this should exit with a failure if not reading
                         *      from stdin and an error occurs, otherwise the
                         *      parser looses track, this is mainly a concern
                         *      for string input, where is makes for very
                         *      confusing behavior*/
			lisp_printf(l, ofp, 0, "%s", prompt);
			if (!(ret = reader(l, lisp_get_input(l))))
				break;
			if (!(ret = eval(l, 0, ret, l->top_env)))
				break;
			lisp_printf(l, ofp, 0, "%S\n", ret);
			l->gc_stack_used = 0;
		}
	}
	l->gc_stack_used = 0;
	l->recover_init = 0;
	return r;
}

int main_lisp_env(lisp_t * l, int argc, char **argv)
{
	int i, stdin_off = 0;
	lisp_cell_t *ob = l->nil;
	if (!l)
		return -1;
	for (i = argc - 1; i + 1; i--)	/*add command line args to list */
		if (!(ob = cons(l, mk_str(l, lstrdup_or_abort(argv[i])), ob)))
			return -1;
	if (!lisp_extend_top(l, lisp_intern(l, lstrdup_or_abort("args")), ob))
		return -1;

        lisp_add_cell(l, "*version*",           mk_str(l, lstrdup_or_abort(XSTRINGIFY(VERSION))));
        lisp_add_cell(l, "*commit*",            mk_str(l, lstrdup_or_abort(XSTRINGIFY(VCS_COMMIT))));
        lisp_add_cell(l, "*repository-origin*", mk_str(l, lstrdup_or_abort(XSTRINGIFY(VCS_ORIGIN))));

	for (i = 1; i < argc; i++)
		switch (getoptions(l, argv[i], argv[0])) {
		case OPTS_SWITCH:
			break;
		case OPTS_IN_STDIN:	/*read from standard input */
			lisp_log_note(l, "'input-file 'stdin");
			io_close(lisp_get_input(l));
			if (lisp_set_input(l, io_fin(stdin)) < 0)
				return perror("stdin"), -1;
			if (lisp_repl(l, l->prompt_on ? "> " : "", l->editor_on) < 0)
				return -1;
			io_close(lisp_get_input(l));
			lisp_set_input(l, NULL);
			stdin_off = 1;
			break;
		case OPTS_IN_FILE_NEXT_ARG:
			if (!(++i < argc))
				return fprintf(stderr, "-i and -- expects file\n"), -1;
			/*--- fall through ---*/
		case OPTS_IN_FILE:	/*read from a file */
			lisp_log_note(l, "'input-file \"%s\"", argv[i]);
			io_close(lisp_get_input(l));
			if (lisp_set_input(l, io_fin(fopen(argv[i], "rb"))) < 0)
				return perror(argv[i]), -1;
			if (lisp_repl(l, "", 0) < 0)
				return -1;
			io_close(lisp_get_input(l));
			lisp_set_input(l, NULL);
			stdin_off = 1;
			break;
		case OPTS_IN_STRING:	/*evaluate a string */
			lisp_log_note(l, "'input-string \"%s\"", argv[i]);
			io_close(lisp_get_input(l));
			if (!(++i < argc))
				return fprintf(stderr, "-e expects arg\n"), -1;
			if (lisp_set_input(l, io_sin(argv[i], strlen(argv[i]))) < 0)
				return perror(argv[i]), -1;
			if (lisp_repl(l, "", 0) < 0)
				return -1;
			io_close(lisp_get_input(l));
			lisp_set_input(l, NULL);
			stdin_off = 1;
			break;
		case OPTS_OUT_FILE:	/*change the file to write to */
			lisp_log_note(l, "'output-file \"%s\"", argv[i]);
			if (!(++i < argc))
				return fprintf(stderr, "-o expects arg\n"), -1;
			io_close(lisp_get_output(l));
			lisp_set_output(l, NULL);
			if (lisp_set_output(l, io_fout(fopen(argv[i], "wb"))) < 0)
				return perror(argv[i]), -1;
			break;
		case OPTS_ERROR:
		default:
			exit(-1);
		}
	if (!stdin_off) {
		lisp_log_note(l, "\"%s\"", "reading from stdin");
		if (lisp_repl(l, l->prompt_on ? "> " : "", l->editor_on) < 0)
			return -1;
	}
	lisp_destroy(l);
	return 0;
}

int main_lisp(int argc, char **argv)
{
	lisp_t *l;
	if (!(l = lisp_init()))
		return -1;
	return main_lisp_env(l, argc, argv);
}

/** @file       subr.c
 *  @brief      The liblisp interpreter built in subroutines
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *
 *  @todo Documentation for each function as it behaves at the lisp interpreter
 *        level could be put next to each subroutine, this would help in
 *        keeping them in sync.  
 *  @todo Functions for hash: hash-keys, hash-values, hash-foreach, index
 *  arbitrary expressions by serializing them first
 *  @todo Functions for mutation of data structures, such as strings **/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* X-Macro of primitive functions and their names; basic built in subr
 * The format is:
 *       primitive subroutine
 *       argument length (only if the validate string is not NULL)
 *       validate string (if NULL it is not used)
 *       documentation string (if NULL then there is no doc-string)
 *       subroutine name (as it appears with in the interpreter) */
#define SUBROUTINE_XLIST\
	X("all-symbols", subr_all_syms,  "",     "get a hash of all the symbols encountered so far")\
	X("apply",       subr_apply,     NULL,   "apply a function to an argument list")\
	X("assoc",       subr_assoc,     "A c",  "lookup a variable in an 'a-list'")\
	X("base",        subr_base,      "d d",  "convert a integer into a string in a base")\
	X("car",         subr_car,       "c",    "return the first object in a list")\
	X("cdr",         subr_cdr,       "c",    "return every object apart from the first in a list")\
	X("closed?",     subr_is_closed, NULL,   "is a object closed?")\
	X("close",       subr_close,     "P",    "close a port, invalidating it")\
	X("coerce",      subr_coerce,    NULL,   "coerce a variable from one type to another")\
	X("cons",        subr_cons,      "A A",  "allocate a new cons cell with two arguments")\
	X("define-eval", subr_define_eval, "s A", "extend the top level environment with a computed symbol")\
	X("depth",       subr_depth,     "",      "get the current evaluation depth")\
	X("environment", subr_environment, "",    "get the current environment")\
	X("eof?",        subr_eofp,      "P",    "is the EOF flag set on a port?")\
	X("eq",          subr_eq,        "A A",  "equality operation")\
	X("eval",        subr_eval,      NULL,   "evaluate an expression")\
	X("ferror",      subr_ferror,    "P",    "is the error flag set on a port")\
	X("flush",       subr_flush,     NULL,   "flush a port")\
	X("foldl",       subr_foldl,    "x c",  "left fold; reduce a list given a function")\
	X("format",      subr_format,    NULL,   "print a string given a format and arguments")\
	X("get-char",    subr_getchar,   "i",    "read in a character from a port")\
	X("get-delim",   subr_getdelim,  "i C",  "read in a string delimited by a character from a port")\
	X("getenv",      subr_getenv,    "Z",    "get an environment variable (not thread safe)")\
	X("get-io-str",  subr_get_io_str,"P",    "get a copy of a string from an IO string port")\
	X("hash-create", subr_hash_create,   NULL,   "create a new hash")\
	X("hash-info",   subr_hash_info,     "h",    "get information about a hash")\
	X("hash-insert", subr_hash_insert,   "h Z A", "insert a variable into a hash")\
	X("hash-lookup", subr_hash_lookup,   "h Z",  "loop up a variable in a hash")\
	X("input?",      subr_inp,       "A",    "is an object an input port?")\
	X("length",      subr_length,    "A",    "return the length of a list or string")\
	X("list",        subr_list,      NULL,   "create a list from the arguments")\
	X("match",       subr_match,     "Z Z",  "perform a primitive match on a string")\
	X("open",        subr_open,      "d Z",  "open a port (either a file or a string) for reading *or* writing")\
	X("output?",     subr_outp,      "A",    "is an object an output port?")\
	X("print",       subr_print,     "o A",  "print out an s-expression")\
	X("put-char",    subr_putchar,   "o d",  "write a character to a output port")\
	X("put",         subr_puts,      "o Z",  "write a string to a output port")\
	X("raw",         subr_raw,       "A",    "get the raw value of an object")\
	X("read",        subr_read,      "I",    "read in an s-expression from a port or a string")\
	X("remove",      subr_remove,    "Z",    "remove a file")\
	X("rename",      subr_rename,    "Z Z",  "rename a file")\
	X("reverse",     subr_reverse,   NULL,   "reverse a string, list or hash")\
	X("scar",        subr_scar,      "Z",    "return the first character in a string")\
	X("scdr",        subr_scdr,      "Z",    "return a string excluding the first character")\
	X("scons",       subr_scons,     "Z Z",  "concatenate two string")\
	X("seek",        subr_seek,      "P d d", "perform a seek on a port (moving the port position indicator)")\
	X("set-car!",    subr_setcar,    "c A",  "destructively set the first cell of a cons cell")\
	X("set-cdr!",    subr_setcdr,    "c A",  "destructively set the second cell of a cons cell")\
	X("signal",      subr_signal,     "d",    "raise a signal")\
	X("&",           subr_band,      "d d",  "bit-wise and of two integers")\
	X("~",           subr_binv,      "d",    "bit-wise inversion of an integers")\
	X("|",           subr_bor,       "d d",  "bit-wise or of two integers")\
	X("^",           subr_bxor,      "d d",  "bit-wise xor of two integers")\
	X("/",           subr_div,       "a a",  "divide operation")\
	X("=",           subr_eq,        "A A",  "equality operation")\
	X(">",           subr_greater,   NULL,   "greater operation")\
	X("<",           subr_less,      NULL,   "less than operation")\
	X("%",           subr_mod,       "d d",  "modulo operation")\
	X("*",           subr_prod,      "a a",  "multiply two numbers")\
	X("-",           subr_sub,       "a a",  "subtract two numbers")\
	X("+",           subr_sum,       "a a",  "add two numbers")\
	X("substring",   subr_substring, NULL,   "create a substring from a string")\
	X("tell",        subr_tell,      "P",    "return the position indicator of a port")\
	X("top-environment", subr_top_env, "",   "return the top level environment")\
	X("trace!",      subr_trace,     "d",    "set the log level, from no errors printed, to copious debugging information")\
	X("tr",          subr_tr,        "Z Z Z Z", "translate a string given a format and mode")\
	X("type-of",     subr_typeof,    "A",    "return an integer representing the type of an object")

#define X(NAME, SUBR, VALIDATION, DOCSTRING) static lisp_cell_t * SUBR (lisp_t *l, lisp_cell_t *args);
SUBROUTINE_XLIST /*function prototypes for all of the built-in subroutines*/
#undef X

#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(NAME, DOCSTRING), SUBR },
static const lisp_module_subroutines_t primitives[] = {
        SUBROUTINE_XLIST /*all of the subr functions*/
        {NULL, NULL, NULL, NULL} /*must be terminated with NULLs*/
};
#undef X

/**< X-Macros of all built in integers*/
#define INTEGER_XLIST\
	X("*seek-cur*",     SEEK_CUR)     X("*seek-set*",    SEEK_SET)\
	X("*seek-end*",     SEEK_END)	  X("*integer*",      INTEGER)\
	X("*symbol*",       SYMBOL)       X("*cons*",         CONS)\
	X("*string*",       STRING)       X("*hash*",         HASH)\
	X("*io*",           IO)           X("*float*",        FLOAT)\
       	X("*procedure*",    PROC)         X("*primitive*",    SUBR)\
	X("*f-procedure*",  FPROC)        X("*file-in*",      IO_FIN)\
	X("*file-out*",     IO_FOUT)      X("*string-in*",    IO_SIN)\
 	X("*string-out*",   IO_SOUT)      X("*user-defined*", USERDEF)\
	X("*eof*",          EOF)          X("*sig-abrt*",     SIGABRT)\
	X("*sig-fpe*",      SIGFPE)       X("*sig-ill*",      SIGILL)\
	X("*sig-int*",      SIGINT)       X("*sig-segv*",     SIGSEGV)\
	X("*sig-term*",     SIGTERM)

#define X(NAME, VAL) { NAME, VAL }, 
/**@brief A list of all integer values to be made available to the
 *        interpreter as lisp objects */
static const struct integer_list { char *name; intptr_t val; } integers[] = {
        INTEGER_XLIST
        {NULL, 0}
};
#undef X

#define X(CNAME, LNAME) static lisp_cell_t _ ## CNAME = { SYMBOL, 0, 1, 0, 0, .p[0].v = LNAME};
CELL_XLIST /*structs for special cells*/
#undef X

#define X(CNAME, NOT_USED) static lisp_cell_t * CNAME = & _ ## CNAME;
CELL_XLIST /*pointers to structs for special cells*/
#undef X

#define X(CNAME, NOT_USED) { & _ ## CNAME },
/**@brief a list of all the special symbols**/
static const struct special_cell_list { lisp_cell_t *internal; } special_cells[] = {
        CELL_XLIST 
        { NULL }
};
#undef X

#define X(FNAME, IGNORE) lisp_cell_t *gsym_ ## FNAME (void) { return FNAME ; }
CELL_XLIST /**< defines functions to get a lisp "cell" for the built in special symbols*/
#undef X

static lisp_cell_t *forced_add_symbol(lisp_t *l, lisp_cell_t *ob) { assert(l && ob); 
        assert(hash_lookup(get_hash(l->all_symbols), get_sym(ob)) == NULL);
        if(hash_insert(get_hash(l->all_symbols), get_sym(ob), ob) < 0) return NULL;
        return l->tee;
}

lisp_t *lisp_init(void) {
        lisp_t *l;
        io_t *ifp, *ofp, *efp;
        unsigned i;
        if(!(l = calloc(1, sizeof(*l))))  goto fail;
        if(!(ifp = io_fin(stdin)))        goto fail;
        if(!(ofp = io_fout(stdout)))      goto fail;
        if(!(efp = io_fout(stderr)))      goto fail;

	lisp_set_log_level(l, LISP_LOG_LEVEL_ERROR);

        l->gc_off = 1;
        if(!(l->buf = calloc(DEFAULT_LEN, 1))) goto fail;
        l->buf_allocated = DEFAULT_LEN;
        if(!(l->gc_stack = calloc(DEFAULT_LEN, sizeof(*l->gc_stack)))) 
                goto fail;
        l->gc_stack_allocated = DEFAULT_LEN;

#define X(CNAME, LNAME) l-> CNAME = CNAME;
CELL_XLIST
#undef X
        assert(MAX_RECURSION_DEPTH < INT_MAX);

        /* The lisp init function is now ready to add built in subroutines
         * and other variables, the order in which is does this matters. */
        if(!(l->all_symbols = mk_hash(l, hash_create(LARGE_DEFAULT_LEN)))) 
                goto fail;
        if(!(l->top_env = cons(l, cons(l, gsym_nil(), gsym_nil()),gsym_nil())))
                goto fail;
        if(!(l->top_hash = mk_hash(l, hash_create(LARGE_DEFAULT_LEN))))
                goto fail;
         set_cdr(l->top_env, cons(l, l->top_hash, cdr(l->top_env)));

        /* Special care has to be taken with the input and output objects
         * as they can change throughout the interpreters lifetime, they
         * should only be set by accessor functions*/
        if(!(l->input   = mk_io(l, ifp))) goto fail;
        if(!(l->output  = mk_io(l, ofp))) goto fail;
        if(!(l->logging = mk_io(l, efp))) goto fail;
        if(!(l->empty_docstr = mk_str(l, lstrdup_or_abort("")))) goto fail;

        l->input->uncollectable = l->output->uncollectable = l->logging->uncollectable = 1;

        /* These are the ports currently used by the interpreter for default
         * input, output and error reporting*/
        if(!lisp_add_cell(l, "*input*",  l->input))   goto fail;
        if(!lisp_add_cell(l, "*output*", l->output))  goto fail;
        if(!lisp_add_cell(l, "*error*",  l->logging)) goto fail;
        /* These are the ports representing stdin/stdout/stderr of the C
         * environment*/
        if(!lisp_add_cell(l, "*stdin*",  mk_io(l, io_fin(stdin))))   goto fail;
        if(!lisp_add_cell(l, "*stdout*", mk_io(l, io_fout(stdout)))) goto fail;
        if(!lisp_add_cell(l, "*stderr*", mk_io(l, io_fout(stderr)))) goto fail;

        for(i = 0; special_cells[i].internal; i++) { /*add special cells*/
                if(!forced_add_symbol(l, special_cells[i].internal))
                        goto fail;
                if(!lisp_extend_top(l, special_cells[i].internal, 
                                  special_cells[i].internal))
                        goto fail;
        }

        for(i = 0; integers[i].name; i++) /*add all integers*/
                if(!lisp_add_cell(l, integers[i].name, 
                                        mk_int(l, integers[i].val)))
                        goto fail;
	lisp_add_module_subroutines(l, primitives, 0);
        l->gc_off = 0;
        return l;
fail:   l->gc_off = 0;
        lisp_destroy(l);
        return NULL;
}

static lisp_cell_t *subr_band(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, get_int(car(args)) & get_int(CADR(args)));
}

static lisp_cell_t *subr_bor(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, get_int(car(args)) | get_int(CADR(args)));
}

static lisp_cell_t *subr_bxor(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, get_int(car(args)) ^ get_int(CADR(args)));
}

static lisp_cell_t *subr_binv(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, ~get_int(car(args)));
}

static lisp_cell_t *subr_sum(lisp_t * l, lisp_cell_t * args)
{
	lisp_cell_t *x = car(args), *y = CADR(args);
	if (is_int(x))
		return mk_int(l, get_int(x) + get_a2i(y));
	return mk_float(l, get_float(x) + get_a2f(y));
}

static lisp_cell_t *subr_sub(lisp_t * l, lisp_cell_t * args)
{
	lisp_cell_t *x = car(args), *y = CADR(args);
	if (is_int(x))
		return mk_int(l, get_int(x) - get_a2i(y));
	return mk_float(l, get_float(x) - get_a2f(y));
}

static lisp_cell_t *subr_prod(lisp_t * l, lisp_cell_t * args)
{
	lisp_cell_t *x = car(args), *y = CADR(args);
	if (is_int(x))
		return mk_int(l, get_int(x) * get_a2i(y));
	return mk_float(l, get_float(x) * get_a2f(y));
}

static lisp_cell_t *subr_mod(lisp_t * l, lisp_cell_t * args)
{
	intptr_t dividend, divisor;
	dividend = get_int(car(args));
	divisor = get_int(CADR(args));
	if (!divisor || (dividend == INTPTR_MIN && divisor == -1))
		LISP_RECOVER(l, "\"invalid divisor values\"\n '%S", args);
	return mk_int(l, dividend % divisor);
}

static lisp_cell_t *subr_div(lisp_t * l, lisp_cell_t * args)
{
	lisp_float_t dividend, divisor;
	if (is_int(car(args))) {
		intptr_t dividend, divisor;
		dividend = get_int(car(args));
		divisor = get_a2i(CADR(args));
		if (!divisor || (dividend == INTPTR_MIN && divisor == -1))
			LISP_RECOVER(l, "\"invalid divisor values\"\n '%S", args);
		return mk_int(l, dividend / divisor);
	}
	dividend = get_float(car(args));
	divisor = get_a2f(CADR(args));
	if (divisor == 0.)
		LISP_RECOVER(l, "\"division by zero\"\n '%S", args);
	return mk_float(l, dividend / divisor);
}

static lisp_cell_t *subr_greater(lisp_t * l, lisp_cell_t * args)
{
	lisp_cell_t *x, *y;
	if (!lisp_check_length(args, 2))
		goto fail;
	x = car(args);
	y = CADR(args);
	if (is_arith(x) && is_arith(y)) {
		return (is_floating(x) ? get_float(x) : get_int(x)) >
		    (is_floating(y) ? get_float(y) : get_int(y)) ? gsym_tee() : gsym_nil();
	} else if (is_asciiz(x) && is_asciiz(y)) {
		size_t lx = get_length(x), ly = get_length(y);
		if(lx == ly)
			return memcmp(get_str(x), get_str(y), lx) > 0 ? gsym_tee() : gsym_nil();
		return lx > ly ? gsym_tee() : gsym_nil();
	}
 fail:	LISP_RECOVER(l, "\"expected (number number) or (string string)\"\n '%S", args);
	return gsym_error();
}

static lisp_cell_t *subr_less(lisp_t * l, lisp_cell_t * args)
{
	lisp_cell_t *x, *y;
	if (!lisp_check_length(args, 2))
		goto fail;
	x = car(args);
	y = CADR(args);
	if (is_arith(x) && is_arith(y)) {
		return (is_floating(x) ? get_float(x) : get_int(x)) <
		    (is_floating(y) ? get_float(y) : get_int(y)) ? gsym_tee() : gsym_nil();
	} else if (is_asciiz(x) && is_asciiz(y)) {
		size_t lx = get_length(x), ly = get_length(y);
		if(lx == ly)
			return memcmp(get_str(x), get_str(y), lx) < 0 ? gsym_tee() : gsym_nil();
		return lx < ly ? gsym_tee() : gsym_nil();
	}
 fail:	LISP_RECOVER(l, "\"expected (number number) or (string string)\"\n '%S", args);
	return gsym_error();
}

static lisp_cell_t *subr_eq(lisp_t * l, lisp_cell_t * args)
{
	lisp_cell_t *x, *y;
	x = car(args);
	y = CADR(args);
	if (is_userdef(x) && l->ufuncs[get_user_type(x)].equal)
		return (l->ufuncs[get_user_type(x)].equal) (x, y) ? gsym_tee() : gsym_nil();
	if (get_int(x) == get_int(y))
		return gsym_tee();
	if (is_floating(x) && is_floating(y))
		return get_float(x) == get_float(y) ? gsym_tee() : gsym_nil();
	if (is_str(x) && is_str(y)) {
		size_t lx = get_length(x), ly = get_length(y);
		if(lx == ly)
			return !memcmp(get_str(x), get_str(y), lx) ? gsym_tee() : gsym_nil();
	}
	return gsym_nil();
}

static lisp_cell_t *subr_cons(lisp_t * l, lisp_cell_t * args)
{
	return cons(l, car(args), CADR(args));
}

static lisp_cell_t *subr_car(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return CAAR(args);
}

static lisp_cell_t *subr_cdr(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return CDAR(args);
}

static lisp_cell_t *subr_setcar(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	set_car(car(args), CADR(args));
	return car(args);
}

static lisp_cell_t *subr_setcdr(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	lisp_cell_t *c = car(args);
	set_cdr(c, CADR(args));
	return car(args);
}

static lisp_cell_t *subr_list(lisp_t * l, lisp_cell_t * args)
{
	size_t i;
	lisp_cell_t *op, *head;
	if (lisp_check_length(args, 0))
		return gsym_nil();
	head = op = cons(l, car(args), gsym_nil());
	args = cdr(args);
	for (i = 1; !is_nil(args); args = cdr(args), op = cdr(op), i++)
		set_cdr(op, cons(l, car(args), gsym_nil()));
	return head;
}

static lisp_cell_t *subr_match(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return match(get_sym(car(args)), get_sym(CADR(args))) ? gsym_tee() : gsym_nil();
}

static lisp_cell_t *subr_scons(lisp_t * l, lisp_cell_t * args)
{
	char *ret;
	ret = CONCATENATE(get_str(car(args)), get_str(CADR(args)));
	return mk_str(l, ret);
}

static lisp_cell_t *subr_scar(lisp_t * l, lisp_cell_t * args)
{
	char c[2] = { '\0', '\0' };
	c[0] = get_str(car(args))[0];
	return mk_str(l, lisp_strdup(l, c));
}

static lisp_cell_t *subr_scdr(lisp_t * l, lisp_cell_t * args)
{
	if (!(get_str(car(args))[0]))
		mk_str(l, lisp_strdup(l, ""));
	return mk_str(l, lisp_strdup(l, &get_str(car(args))[1]));;
}

static lisp_cell_t *subr_eval(lisp_t * l, lisp_cell_t * args)
{
	lisp_cell_t *x = NULL;
	int restore_used, r, errors_halt = l->errors_halt;
	jmp_buf restore;
	l->errors_halt = 0;
	if (l->recover_init) {
		memcpy(restore, l->recover, sizeof(jmp_buf));
		restore_used = 1;
	}
	l->recover_init = 1;
	if ((r = setjmp(l->recover))) {
		LISP_RECOVER_RESTORE(restore_used, l, restore);
		l->errors_halt = errors_halt;
		return gsym_error();
	}

	if (lisp_check_length(args, 1))
		x = eval(l, l->cur_depth, car(args), l->top_env);
	if (lisp_check_length(args, 2)) {
		if (!is_cons(CADR(args)))
			LISP_RECOVER(l, "\"expected a-list\"\n '%S", args);
		x = eval(l, l->cur_depth, car(args), CADR(args));
	}

	LISP_RECOVER_RESTORE(restore_used, l, restore);
	if (!x)
		LISP_RECOVER(l, "\"expected (expr) or (expr environment)\"\n '%S", args);
	l->errors_halt = errors_halt;
	return x;
}

static lisp_cell_t *subr_trace(lisp_t * l, lisp_cell_t * args)
{
	lisp_log_level level = get_int(car(args));
	switch(level) {
	case LISP_LOG_LEVEL_OFF:
	case LISP_LOG_LEVEL_ERROR:
	case LISP_LOG_LEVEL_NOTE:
	case LISP_LOG_LEVEL_DEBUG:
		lisp_set_log_level(l, level);
		break;
	default:
		LISP_RECOVER(l, "%r\"invalid log level\"\n %m%d%t", (intptr_t)level);
	}
	return gsym_tee();
}

static lisp_cell_t *subr_length(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, (intptr_t) get_length(car(args)));
}

static lisp_cell_t *subr_inp(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return is_in(car(args)) ? gsym_tee() : gsym_nil();
}

static lisp_cell_t *subr_outp(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return is_out(car(args)) ? gsym_tee() : gsym_nil();
}

static lisp_cell_t *subr_open(lisp_t * l, lisp_cell_t * args)
{ /**@todo fix validation string for IO_SOUT*/
	io_t *ret = NULL;
	char *file  = get_str(CADR(args));
	size_t flen = get_length(CADR(args));
	intptr_t type = get_int(car(args));
	switch (type) { 
	case IO_FIN:
		ret = io_fin(fopen(file, "rb"));
		break;
	case IO_FOUT:
		ret = io_fout(fopen(file, "wb"));
		break;
	case IO_SIN:
		ret = io_sin(file, flen);
		break;
	case IO_SOUT:
		ret = io_sout(2);
		break;
	default:
		LISP_RECOVER(l, "\"invalid operation %d\"\n '%S", get_int(car(args)), args);
	}
	return ret == NULL ? gsym_nil() : mk_io(l, ret);
}

static lisp_cell_t *subr_get_io_str(lisp_t *l, lisp_cell_t *args)
{ /**@todo fix for binary data */
	if(!io_is_string(get_io(car(args))))
		LISP_RECOVER(l, "\"get string only works on string output IO ports\"", args);
	return mk_str(l, lisp_strdup(l, io_get_string(get_io(car(args)))));
}

static lisp_cell_t *subr_getchar(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, io_getc(get_io(car(args))));
}

static lisp_cell_t *subr_getdelim(lisp_t * l, lisp_cell_t * args)
{
	char *s;
	int ch = is_asciiz(CADR(args)) ? get_str(CADR(args))[0] : get_int(CADR(args));
	return (s = io_getdelim(get_io(car(args)), ch)) ? mk_str(l, s) : gsym_nil();
}

static lisp_cell_t *subr_read(lisp_t * l, lisp_cell_t * args)
{
	lisp_cell_t *x;
	int restore_used, r, errors_halt = l->errors_halt;
	jmp_buf restore;
	char *s;
	l->errors_halt = 0;
	if (l->recover_init) {	/*store exception state */
		memcpy(restore, l->recover, sizeof(jmp_buf));
		restore_used = 1;
	}
	l->recover_init = 1;
	if ((r = setjmp(l->recover))) {	/*handle exception in reader */
		LISP_RECOVER_RESTORE(restore_used, l, restore);
		l->errors_halt = errors_halt;
		return gsym_error();
	}
	s = NULL;
	x = NULL;
	io_t *i = NULL;
	if (!(i = is_in(car(args)) ? get_io(car(args)) : io_sin(get_str(car(args)), get_length(car(args)))))
		LISP_HALT(l, "\"%s\"", "out of memory");
	x = (x = reader(l, i)) ? x : gsym_error();
	if (s)
		io_close(i);
	LISP_RECOVER_RESTORE(restore_used, l, restore);
	l->errors_halt = errors_halt;
	return x;
}

static lisp_cell_t *subr_puts(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return io_puts(get_str(CADR(args)), get_io(car(args))) < 0 ? gsym_nil() : CADR(args);
}

static lisp_cell_t *subr_putchar(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return io_putc(get_int(CADR(args)), get_io(car(args))) < 0 ? gsym_nil() : CADR(args);
}

static lisp_cell_t *subr_print(lisp_t * l, lisp_cell_t * args)
{
	return printer(l, get_io(car(args)), CADR(args), 0) < 0 ? gsym_nil() : CADR(args);
}

static lisp_cell_t *subr_flush(lisp_t * l, lisp_cell_t * args)
{
	if (lisp_check_length(args, 0))
		return mk_int(l, fflush(NULL));
	if (lisp_check_length(args, 1) && is_io(car(args)))
		return io_flush(get_io(car(args))) ? gsym_nil() : gsym_tee();
	LISP_RECOVER(l, "\"expected () or (io)\"\n '%S", args);
	return gsym_error();
}

static lisp_cell_t *subr_tell(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, io_tell(get_io(car(args))));
}

static lisp_cell_t *subr_seek(lisp_t * l, lisp_cell_t * args)
{
	switch (get_int(CADR(cdr(args)))) {
	case SEEK_SET:
	case SEEK_CUR:
	case SEEK_END:
		break;
	default:
		LISP_RECOVER(l, "\"invalid enum option\"\n '%S", args);
	}
	return mk_int(l, io_seek(get_io(car(args)), get_int(CADR(args)), get_int(CADR(cdr(args)))));
}

static lisp_cell_t *subr_eofp(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return io_eof(get_io(car(args))) ? gsym_tee() : gsym_nil();
}

static lisp_cell_t *subr_ferror(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return io_error(get_io(car(args))) ? gsym_tee() : gsym_nil();
}

static lisp_cell_t *subr_remove(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return remove(get_str(car(args))) ? gsym_nil() : gsym_tee();
}

static lisp_cell_t *subr_rename(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return rename(get_str(car(args)), get_str(CADR(args))) ? gsym_nil() : gsym_tee();
}

static lisp_cell_t *subr_hash_lookup(lisp_t * l, lisp_cell_t * args)
{ /*arbitrary expressions could be used as keys if they are serialized to strings first*/
	UNUSED(l);
	lisp_cell_t *x;
	return (x = hash_lookup(get_hash(car(args)), get_sym(CADR(args)))) ? x : gsym_nil();
}

static lisp_cell_t *subr_hash_insert(lisp_t * l, lisp_cell_t * args)
{
	if (hash_insert(get_hash(car(args)),
			get_sym(CADR(args)), cons(l, CADR(args), CADR(cdr(args)))))
		LISP_HALT(l, "%s", "out of memory");
	return car(args);
}

static lisp_cell_t *subr_hash_create(lisp_t * l, lisp_cell_t * args)
{
	hash_table_t *ht = NULL;
	if (get_length(args) % 2)
		goto fail;
	if (!(ht = hash_create(SMALL_DEFAULT_LEN)))
		LISP_HALT(l, "%s", "out of memory");
	for (; !is_nil(args); args = cdr(cdr(args))) {
		if (!is_asciiz(car(args)))
			goto fail;
		if (hash_insert(ht, get_sym(car(args)), cons(l, car(args), CADR(args))) < 0)
			LISP_HALT(l, "%s", "out of memory");
	}
	return mk_hash(l, ht);
 fail:	hash_destroy(ht);
	LISP_RECOVER(l, "\"expected ({symbol any}*)\"\n '%S", args);
	return gsym_error();
}

static lisp_cell_t *subr_hash_info(lisp_t * l, lisp_cell_t * args)
{
	hash_table_t *ht = get_hash(car(args));
	return mk_list(l,
		       mk_float(l, hash_get_load_factor(ht)),
		       mk_int(l,   hash_get_replacements(ht)),
		       mk_int(l,   hash_get_collision_count(ht)),
		       mk_int(l,   hash_get_number_of_bins(ht)), NULL);
}

static lisp_cell_t *subr_coerce(lisp_t * l, lisp_cell_t * args)
{
	if (!lisp_check_length(args, 2) && !is_int(car(args)))
		goto fail;
	return lisp_coerce(l, get_int(car(args)), CADR(args));
 fail:	LISP_RECOVER(l, "\"expected (int any)\"\n %S", args);
	return gsym_error();
}

lisp_cell_t *lisp_coerce(lisp_t * l, lisp_type type, lisp_cell_t *from)
{
	char *fltend = NULL;
	intptr_t d = 0;
	size_t i = 0, j;
	lisp_cell_t *x, *y, *head;
	if (type == from->type)
		return from;
	switch (type) {
	case INTEGER:
		if (is_str(from)) {
			if (!is_number(get_str(from)))
				goto fail;
			sscanf(get_str(from), "%" SCNiPTR, &d);
		} else if (is_floating(from)) {	/*float to string */
			d = (intptr_t) get_float(from);
		} else {
			goto fail;
		}
		return mk_int(l, d);
	case CONS:
		if (is_str(from)) {	/*string to list of chars */
			size_t fromlen = get_length(from);
			head = x = cons(l, gsym_nil(), gsym_nil());
			if(!fromlen)
				return cons(l, mk_str(l, lstrdup_or_abort("")), gsym_nil());
			for (i = 0; i < fromlen; i++) {
				/**@todo fix for binary data */
				char c[2] = { '\0', '\0' };
				c[0] = get_str(from)[i];
				y = mk_str(l, lisp_strdup(l, c));
				set_cdr(x, cons(l, y, gsym_nil()));
				x = cdr(x);
			}
			return cdr(head);
		}
		if (is_hash(from)) {	/*hash to list */
			hash_entry_t *cur;
			hash_table_t *h = get_hash(from);
			head = x = cons(l, gsym_nil(), gsym_nil());
			for (j = 0, i = 0; i < h->len; i++)
				if (h->table[i])
					for (cur = h->table[i]; cur; cur = cur->next, j++) {
						lisp_cell_t *tmp = (lisp_cell_t *) cur->val;
						if (!is_cons(tmp))	/*handle special case for all_symbols hash */
							tmp = cons(l, tmp, tmp);
						set_cdr(x, cons(l, tmp, gsym_nil()));
						x = cdr(x);
					}
			return cdr(head);
		}
		break;
	case STRING:
		if (is_int(from)) {		/*int to string */
			char s[64] = "";	/*holds all integer strings */
			sprintf(s, "%" PRIiPTR, get_int(from));
			return mk_str(l, lisp_strdup(l, s));
		}
		if (is_sym(from))		/*symbol to string */
			return mk_str(l, lisp_strdup(l, get_str(from)));
		if (is_floating(from)) {	/*float to string */
			char s[64] = "";	/*holds all float strings */
			sprintf(s, "%e", get_float(from));
			return mk_str(l, lisp_strdup(l, s));
		}
		if (is_cons(from)) {	/*list of chars/ints to string */
			size_t i;
			char *s;
			for (x = from; !is_nil(x); x = cdr(x)) {	/*validate args */
				if (!is_proper_cons(x))
					goto fail;	/*not a proper list */
				if (!is_int(car(x)))
					goto fail;	/*convert only integers */
			}
			x = from;
			if (!(s = calloc(get_length(x) + 1, 1)))
				LISP_HALT(l, "\"%s\"", "out of memory");
			for (i = 0; !is_nil(x); x = cdr(x), i++)
				s[i] = get_int(car(x));
			return mk_str(l, s);
		}
		break;
	case SYMBOL:
		if (is_str(from))
			if (!strpbrk(get_str(from), " ;#()\t\n\r'\"\\"))
				return lisp_intern(l, lisp_strdup(l, get_str(from)));
		break;
	case HASH:
		if (is_cons(from))	/*hash from list */
			return subr_hash_create(l, from);
		break;
	case FLOAT:
		if (is_int(from))	/*int to float */
			return mk_float(l, get_int(from));
		if (is_str(from)) {	/*string to float */
			lisp_float_t d;
			if (!is_fnumber(get_str(from)))
				goto fail;
			d = strtod(get_str(from), &fltend);
			if (!fltend[0])
				return mk_float(l, d);
			else
				goto fail;
		}
	default:
		break;
	}
 fail:	LISP_RECOVER(l, "%r\"invalid conversion\"\n %m%d%t %S", type, from);
	return gsym_error();
}

static lisp_cell_t *subr_assoc(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return lisp_assoc(car(args), CADR(args));
}

static lisp_cell_t *subr_typeof(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, car(args)->type);
}

static lisp_cell_t *subr_close(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	lisp_cell_t *x;
	x = car(args);
	x->close = 1;
	io_close(get_io(x));
	return x;
}

static lisp_cell_t *subr_reverse(lisp_t * l, lisp_cell_t * args)
{
	if (!lisp_check_length(args, 1))
		goto fail;
	if (gsym_nil() == car(args))
		return gsym_nil();
	switch (car(args)->type) {
	case STRING:
		{
			char *s = lisp_strdup(l, get_str(car(args)));
			size_t len;
			if (!s)
				LISP_HALT(l, "\"%s\"", "out of memory");
			if (lisp_check_length(car(args), 0))
				return mk_str(l, s);
			len = get_length(car(args));
			return mk_str(l, breverse(s, len - 1));
		}
		break;
	case CONS:
		{
			lisp_cell_t *x = car(args), *y = gsym_nil();
			if (!is_cons(cdr(x)) && !is_nil(cdr(x)))
				return cons(l, cdr(x), car(x));
			for (; is_cons(x); x = cdr(x))
				y = cons(l, car(x), y);
			if(!is_nil(x))
				LISP_RECOVER(l, "\"cannot reverse list ending in dotted pair\" '%S", args);
			return y;
		}
		break;
	case HASH:
		{ /*only certain hashes are reversible*/
			hash_table_t *old = get_hash(car(args));
			size_t len = hash_get_number_of_bins(old);
			size_t i;
			hash_table_t *new = hash_create(len);
			hash_entry_t *cur;
			for(i = 0; i < old->len; i++)
				if(old->table[i])
					for(cur = old->table[i]; cur; cur = cur->next) {
						lisp_cell_t *key, *val;
						/**@warning weird hash stuff*/
						if(is_cons(cur->val) && is_asciiz(cdr(cur->val))) { 
							key = cdr(cur->val);
							val = car(cur->val);
						} else if(!is_cons(cur->val) && is_asciiz(cur->val)) {
							key = cur->val;
							val = mk_str(l, lisp_strdup(l, cur->key));
						} else {
							goto hfail;
						}
						if(hash_insert(new, get_str(key), cons(l, key, val)) < 0)
							LISP_HALT(l, "\"%s\"", "out of memory");
					}
			return mk_hash(l, new);
hfail:
			hash_destroy(new);
			LISP_RECOVER(l, "\"%s\" '%S", "unreversible hash", car(args));
		}
	default:
		break;
	}
 fail:	LISP_RECOVER(l, "\"expected () (string) (list)\"\n '%S", args);
	return gsym_error();
}

static lisp_cell_t *subr_signal(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return raise(get_int(car(args))) ? gsym_nil() : gsym_tee();
}

static lisp_cell_t *subr_substring(lisp_t * l, lisp_cell_t * args)
{
	intptr_t left, right, tmp;
	char *subs;
	/**@todo sort this function out*/
	if(!get_length(args))
		goto fail;
	if (!(get_length(args) == 2 || get_length(args) == 3)
	    || !is_asciiz(car(args)) || !is_int(CADR(args)))
		goto fail;
	if (get_length(args) == 3 && !is_int(CADDR(args)))
		goto fail;
	left = get_int(CADR(args));
	if (get_length(args) == 2) {
		if (left >= 0) {
			tmp = get_length(car(args));
			left = MIN(left, tmp);
			assert(left < tmp);
			return mk_str(l, lisp_strdup(l, get_str(car(args)) + left));
		} else if (left < 0) {
			left = (int)get_length(car(args)) + left;
			left = MAX(0, left);
			if (!(subs = calloc(left + 1, 1)))
				LISP_HALT(l, "\"%s\"", "out of memory");
			assert((get_length(car(args)) - left) > 0);
			assert((get_str(car(args)) + left) <
			       (get_str(car(args)) + get_length(car(args))));
			memcpy(subs, get_str(car(args)) + left, get_length(car(args)) - left);
			return mk_str(l, subs);
		}
	}
	if (((right = get_int(CADDR(args))) < 0) || left < 0)
		LISP_RECOVER(l, "\"substring lengths must positive\"\n '%S", args);
	left = MIN(left, (int)get_length(car(args)));
	if ((left + right) >= (int)get_length(car(args))) {
		tmp = (right + left) - get_length(car(args));
		right = right - tmp;
		assert((left + right) <= (int)get_length(car(args)));
	}
	if (!(subs = calloc(right + 1, 1)))
		LISP_HALT(l, "\"%s\"", "out of memory");
	memcpy(subs, get_str(car(args)) + left, right);
	return mk_str(l, subs);
fail:   
	LISP_RECOVER(l, "\"expected (string int int?)\"\n '%S", args);
	return gsym_error();
}

static lisp_cell_t *subr_format(lisp_t * l, lisp_cell_t * args)
{
	/** @note This function has a lot of scope for improvement;
         *        colorization, printing different base numbers, leading
         *        zeros on numbers, printing repeated characters and even
         *        string interpolation ("$x" could look up 'x), as well
         *        as printing out escaped strings.
	 *  @bug  What happens if it cannot write to a file!? */
	lisp_cell_t *cret;
	io_t *o = NULL, *t = NULL;
	char *fmt, c, *ts;
	int ret = 0, pchar;
	intptr_t d;
	size_t len;
	len = get_length(args);
	if (len < 1)
		goto argfail;
	if (is_out(car(args))) {
		o = get_io(car(args));
		args = cdr(args);
	}
	if (len < 1 || !is_asciiz(car(args)))
		goto fail;
	if (!(t = io_sout(2)))
		LISP_HALT(l, "%r\"%s\"%t", "out of memory");
	fmt = get_str(car(args));
	args = cdr(args);
	while ((c = *fmt++))
		if (ret == EOF)
			goto fail;
		else if ('%' == c) {
			switch ((c = *fmt++)) {
			case '\0':
				goto fail;
			case '%':
				ret = io_putc(c, t);
				break;
			case 'c':
				if (is_nil(args) || (!is_asciiz(car(args)) && !is_int(car(args))))
					goto fail;
				if (is_int(car(args))) {
					pchar = get_int(car(args));
				} else {	/*must be a character */
					if (!lisp_check_length(car(args), 1))
						goto fail;
					pchar = get_str(car(args))[0];
				}
				ret = io_putc(pchar, t);
				args = cdr(args);
				break;
			case 's':
				if (is_nil(args) || !is_asciiz(car(args)))
					goto fail;
				ret = io_puts(get_str(car(args)), t);
				args = cdr(args);
				break;
			case 'S':
				if (is_nil(args))
					goto fail;
				ret = printer(l, t, car(args), 0);
				args = cdr(args);
				break;
			case 'd':
				if (is_nil(args) || !is_arith(car(args)))
					goto fail;
				ret = io_printd(get_a2i(car(args)), t);
				args = cdr(args);
				break;
			case 'f':
				if (is_nil(args) || !is_arith(car(args)))
					goto fail;
				ret = io_printflt(get_a2f(car(args)), t);
				args = cdr(args);
				break;
			case '@':
				if (is_nil(args) || !is_int(car(args)))
					goto fail;
				d = get_int(car(args));
				if (d < 0 || !(pchar = *fmt++))
					goto fail;
				while (d--)
					io_putc(pchar, t);
				args = cdr(args);
				break;
			case 'x':
				if (is_nil(args) || !is_int(car(args)))
					goto fail;
				if (!(ts = dtostr(get_int(car(args)), 16u)))
					LISP_HALT(l, "%r\"%s\"%t", "out of memory");
				io_puts(ts[0] == '-' ? "-0x" : "0x", t);
				ret = io_puts(ts[0] == '-' ? ts + 1 : ts, t);
				free(ts);
				args = cdr(args);
				break;
			/**@todo 'u' should be a type qualifier, so it can
                         * print out unsigned hexadecimal, octal or decimal
                         * strings*/
			case 'u':
				if (is_nil(args) || !is_int(car(args)))
					goto fail;
				if (!(ts = utostr(get_int(car(args)), 10u)))
					LISP_HALT(l, "%r\"%s\"%t", "out of memory");
				ret = io_puts(ts, t);
				free(ts);
				args = cdr(args);
				break;
				/* case 'o': // octal */
				/* case 'b': // binary */
				/* case '$': // literal '$' */
				/* case '0': case '1': case '2': case '3': // char literal */
			default:
				goto fail;
			}
			/* } else if ('$' == c) { // interpolate */
		} else {
			ret = io_putc(c, t);
		}
	if (!is_nil(args))
		goto fail;
	if (o)
		io_puts(io_get_string(t), o);
	cret = mk_str(l, io_get_string(t));	/*t->p.str is not freed by io_close */
	io_close(t);
	return cret;
 argfail:LISP_RECOVER(l, "\"expected () (io? str any...)\"\n '%S", args);
 fail:	free(io_get_string(t));
	io_close(t);
	/*@todo check if this was a format error, of if this was caused
	 * by a file write failure, if so, then only return what was
	 * rewritten and signal a failure happened somewhere, this
	 * means format should return (error-status "string") not just
	 * "string" */
	LISP_RECOVER(l, "\"format error\"\n %S", args);
	return gsym_error();
}

static lisp_cell_t *subr_tr(lisp_t * l, lisp_cell_t * args)
{
	/**@todo this function needs a lot of improvement, as does the
         *       functions that implement the translation routines*/
	tr_state_t st;
	char *mode, *s1, *s2, *tr, *ret;
	size_t len;
	mode = get_str(car(args));
	s1 = get_str(CADR(args));
	s2 = get_str(CADDR(args));
	tr = get_str(CADDDR(args));
	len = get_length(CADDDR(args));
	memset(&st, 0, sizeof(st));
	switch (tr_init(&st, mode, (uint8_t *) s1, (uint8_t *) s2)) {
	case TR_OK:
		break;
	case TR_EINVAL:
		LISP_RECOVER(l, "\"invalid mode\"\n \"%s\"", mode);
	case TR_DELMODE:
		LISP_RECOVER(l, "\"set 2 not NULL\"\n '%S", args);
	default:
		LISP_RECOVER(l, "\"unknown tr error\"\n '%S", args);
	}
	if (!(ret = calloc(len + 1, 1)))
		LISP_HALT(l, "\"%s\"", "out of memory");
	tr_block(&st, (uint8_t *) tr, (uint8_t *) ret, len);
	return mk_str(l, ret);
}

static lisp_cell_t *subr_define_eval(lisp_t * l, lisp_cell_t * args)
{
	return lisp_extend_top(l, car(args), CADR(args));
}

static lisp_cell_t *subr_top_env(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return l->top_env;
}

static lisp_cell_t *subr_depth(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_int(l, l->cur_depth);
}

static lisp_cell_t *subr_raw(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, (intptr_t) get_raw(car(args)));
}

static lisp_cell_t *subr_environment(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return l->cur_env;
}

static lisp_cell_t *subr_all_syms(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return l->all_symbols;
}

static lisp_cell_t *subr_getenv(lisp_t * l, lisp_cell_t * args)
{
	char *ret;
	return (ret = getenv(get_str(car(args)))) ? mk_str(l, lisp_strdup(l, ret)) : gsym_nil();
}

static lisp_cell_t *subr_is_closed(lisp_t * l, lisp_cell_t * args)
{
	if (!lisp_check_length(args, 1))
		LISP_RECOVER(l, "%r\"expected (any)\"%t\n '%S", args);
	return is_closed(car(args)) ? gsym_tee() : gsym_nil();
}

static lisp_cell_t *subr_foldl(lisp_t * l, lisp_cell_t * args)
{
	lisp_cell_t *f, *start, *tmp, *ret;
	f = car(args);
	tmp = CADR(args);
	start = eval(l, l->cur_depth, car(tmp), l->cur_env);
	tmp = cdr(tmp);
	for (ret = start; is_cons(tmp); tmp = cdr(tmp)) {
		ret = mk_list(l, gsym_quote(), ret, NULL);
		ret = eval(l, l->cur_depth, mk_list(l, f, car(tmp), ret, NULL), l->cur_env);
	}
	if(!is_nil(tmp))
		LISP_RECOVER(l, "%r\"cannot foldl a dotted pair\" '%S", args);
	return ret;
}

static lisp_cell_t *subr_base(lisp_t * l, lisp_cell_t * args)
{
	intptr_t base = get_int(CADR(args));
	if (base < 2 || base > 36)
		LISP_RECOVER(l, "%r\"base < 2 || base > 36\"%t\n '%S", args);
	return mk_str(l, dtostr(get_int(car(args)), base));
}

/*@note apply-partially https://www.gnu.org/software/emacs/manual/html_node/elisp/Calling-Functions.html */
static lisp_cell_t *subr_apply(lisp_t * l, lisp_cell_t * args)
{
	lisp_cell_t *head = args, *prev = args;
	prev = head = args;
	for(args = cdr(args); is_cons(args); prev = args, args = cdr(args))
		if(is_nil(cdr(args)) && is_cons(car(args)))
			set_cdr(prev, car(args));
	return eval(l, l->cur_depth, head, l->cur_env);
}

/** @file       util.c
 *  @brief      utility functions that are used within the liblisp project, but
 *              would also be of use throughout the project.
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com**/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

void pfatal(const char *msg, const char *file, const char *func, long line)
{
	assert(msg && file);
	if(errno)
		fprintf(stderr, "(error \"%s\" \"%s\" \"%s\" %ld)\n", msg, file, func, line);
	else
		fprintf(stderr, "(error \"%s\" \"%s\" \"%s\" \"%s\" %ld)\n", msg, strerror(errno),file, func, line);
	abort();
}

char *lstrdup(const char *s)
{
	assert(s);
	char *str;
	if (!(str = malloc(strlen(s) + 1)))
		return NULL;
	strcpy(str, s);
	return str;
}

char *lstrdup_or_abort(const char *s)
{
	char *r = lstrdup(s);
	if(!r)
		FATAL("string duplication failed");
	return r;
}

static int matcher(char *pat, char *str, size_t depth, jmp_buf * bf)
{
	if (!depth)
		longjmp(*bf, -1);
	if (!str)
		return 0;
 again:
        switch (*pat) {
	case '\0':
		return !*str;
	case '*':
		return matcher(pat + 1, str, depth - 1, bf) || (*str && matcher(pat, str + 1, depth - 1, bf));
	case '.':
		if (!*str)
			return 0;
		pat++;
		str++;
		goto again;
	case '\\':
		if (!*(pat + 1))
			return -1;
		if (!*str)
			return 0;
		pat++;		/*fall through */
	default:
		if (*pat != *str)
			return 0;
		pat++;
		str++;
		goto again;
	}
}

int match(char *pat, char *str)
{
	assert(pat && str);
	jmp_buf bf;
	if (setjmp(bf))
		return -1;
	return matcher(pat, str, LARGE_DEFAULT_LEN, &bf);
}

uint32_t djb2(const char *s, size_t len)
{
	assert(s);
	uint32_t h = 5381;	/*magic number this hash uses, it just is */
	size_t i = 0;
	for (i = 0; i < len; s++, i++)
		h = ((h << 5) + h) + (*s);
	return h;
}

char *getadelim(FILE * in, int delim)
{
	assert(in);
	io_t io_in;
	memset(&io_in, 0, sizeof(io_in));
	io_in.p.file = in;
	io_in.type = IO_FIN;
	return io_getdelim(&io_in, delim);
}

char *getaline(FILE * in)
{
	assert(in);
	return getadelim(in, '\n');
}

char *lstrcatend(char *dest, const char *src)
{
	assert(dest && src);
	size_t sz = strlen(dest);
	strcpy(dest + sz, src);
	return dest + sz + strlen(src);
}

char *vstrcatsep(const char *separator, const char *first, ...)
{
	size_t len, seplen, num = 0;
	char *retbuf, *va, *p;
	va_list argp1, argp2;

	if (!separator || !first)
		return NULL;
	len = strlen(first);
	seplen = strlen(separator);

	va_start(argp1, first);
	va_copy(argp2, argp1);
	while ((va = va_arg(argp1, char *)))
		 num++, len += strlen(va);
	va_end(argp1);

	len += (seplen * num);
	if (!(retbuf = malloc(len + 1)))
		return NULL;
	retbuf[0] = '\0';
	p = lstrcatend(retbuf, first);
	va_start(argp2, first);
	while ((va = va_arg(argp2, char *)))
		 p = lstrcatend(p, separator), p = lstrcatend(p, va);
	va_end(argp2);
	return retbuf;
}

int unbalanced(const char *sexpr, char lpar, char rpar)
{
	assert(sexpr);
	int bal = 0, c;
	while ((c = *sexpr++))
		if (c == lpar)
			bal++;
		else if (c == rpar)
			bal--;
		else if (c == '"') {
			while ((c = *sexpr++)) {
				if (c == '\\' && '"' == *sexpr)
					sexpr++;
				else if (c == '"')
					break;
				else
					continue;
			}
			if (!c)
				return bal;
		} else
			continue;
	return bal;
}

int is_number(const char *buf)
{
	assert(buf);
	char conv[] = "0123456789abcdefABCDEF";
	if (!buf[0])
		return 0;
	if (buf[0] == '-' || buf[0] == '+')
		buf++;
	if (!buf[0])
		return 0;
	if (buf[0] == '0') {	/*shorten the conv table depending on numbers base */
		if (buf[1] == 'x' || buf[1] == 'X')
			conv[22] = '\0', buf += 2;
		else
			conv[8] = '\0';
	} else {
		conv[10] = '\0';
	}
	if (!buf[0])
		return 0;
	return buf[strspn(buf, conv)] == '\0';
}

int is_fnumber(const char *buf)
{
	assert(buf);
	size_t i;
	char conv[] = "0123456789";
	if (!buf[0])
		return 0;
	if (buf[0] == '-' || buf[0] == '+')
		buf++;
	if (!buf[0])
		return 0;
	i = strspn(buf, conv);
	if (buf[i] == '\0')
		return 1;
	if (buf[i] == 'e')
		goto expon;	/*got check for valid exponentiation */
	if (buf[i] != '.')
		return 0;
	buf = buf + i + 1;
	i = strspn(buf, conv);
	if (buf[i] == '\0')
		return 1;
	if (buf[i] != 'e' && buf[i] != 'E')
		return 0;
 expon: 
        buf = buf + i + 1;
	if (buf[0] == '-' || buf[0] == '+')
		buf++;
	if (!buf[0])
		return 0;
	i = strspn(buf, conv);
	if (buf[i] == '\0')
		return 1;
	return 0;
}

char *breverse(char *s, size_t len)
{
	char c;
	size_t i = 0;
	do {
		c = s[i];
		s[i] = s[len - i];
		s[len - i] = c;
	} while (i++ < (len / 2));
	return s;
}

static const char conv[] = "0123456789abcdefghijklmnopqrstuvwxzy";
char *dtostr(intptr_t d, unsigned base)
{
	assert(base > 1 && base < 37);
	intptr_t i = 0, neg = d;
	uintptr_t x = d;
	char s[64 + 2] = "";
	if (x > INTPTR_MAX)
		x = -x;
	do {
		s[i++] = conv[x % base];
	} while ((x /= base) > 0);
	if (neg < 0)
		s[i++] = '-';
	return lstrdup(breverse(s, i - 1));
}

char *utostr(uintptr_t u, unsigned base)
{
	assert(base > 1 && base < 37);
	uintptr_t i = 0;
	char s[64 + 1] = "";
	do {
		s[i++] = conv[u % base];
	} while ((u /= base));
	return lstrdup(breverse(s, i - 1));
}

/*tr functionality*/

static int tr_getnext(uint8_t ** s)
{
	uint8_t seq[5] = "0000";
	if ((*s)[0] == '\0') {
	} else if ((*s)[0] == '\\') {
		switch ((*s)[1]) {
		case 'a':
			(*s) += 2;
			return '\a';
		case 'b':
			(*s) += 2;
			return '\b';
		case 'f':
			(*s) += 2;
			return '\f';
		case 'n':
			(*s) += 2;
			return '\n';
		case 'r':
			(*s) += 2;
			return '\r';
		case 't':
			(*s) += 2;
			return '\t';
		case 'v':
			(*s) += 2;
			return '\v';
		case '-':
			(*s) += 2;
			return '-';
		case '\\':
			(*s) += 2;
			return '\\';
		case '\0':
			return -1;
		default:
			break;
		}
		if (strspn((char *)(*s) + 1, "01234567") > 2) {
			int r = strtol((char *)memcpy(seq + 1, (*s) + 1, sizeof(seq) - 1) - 1, NULL, 8) & 0377;
			*s += 4;
			return r;
		}
	} else {
		return (*s)++, (*s - 1)[0];
	}
	return -1;
}

int tr_init(tr_state_t * tr, char *mode, uint8_t * s1, uint8_t * s2)
{
	unsigned i = 0;
	int c, d, cp, dp;
	assert(tr && mode && s1);	/*s2 is optional */
	memset(tr, 0, sizeof(*tr));
	while ((c = mode[i++]))
		switch (c) {
		case 'x':
			break;
		case 'c':
			tr->compliment_seq = 1;
			break;
		case 's':
			tr->squeeze_seq = 1;
			break;
		case 'd':
			tr->delete_seq = 1;
			break;
		case 't':
			tr->truncate_seq = 1;
			break;
		default:
			return TR_EINVAL;
		}

	for (i = 0; i < 256; i++)
		tr->set_tr[i] = i;

	if (tr->delete_seq) {
		if (s2 || tr->truncate_seq)	/*set 2 should be NULL in delete mode */
			return TR_DELMODE;
		while ((dp = tr_getnext(&s1)) > 0)
			tr->set_del[dp] = 1;
		return TR_OK;
	}

	if (tr->truncate_seq) {
		size_t s1l = strlen((char *)s1), s2l = strlen((char *)s2);
		s1[MIN(s2l, s1l)] = '\0';
	}

	c = d = -1;
	while ((cp = tr_getnext(&s1)) > 0) {
		dp = tr_getnext(&s2);
		if (((cp < 0) && (c < 0)) || ((dp < 0) && (d < 0)))
			return TR_EINVAL;
		c = cp;
		d = dp;
		tr->set_tr[c] = d;
		if (tr->squeeze_seq)
			tr->set_squ[c] = 1;
	}
	return TR_OK;
}

int tr_char(tr_state_t * tr, uint8_t c)
{				/*return character to emit, -1 otherwise */
	assert(tr);
	if ((c == tr->previous_char) && tr->squeeze_seq && tr->set_squ[c])
		return -1;
	tr->previous_char = c;
	if (tr->delete_seq)
		return tr->set_del[c] ? -1 : c;
	return tr->set_tr[c];
}

size_t tr_block(tr_state_t * tr, uint8_t * in, uint8_t * out, size_t len)
{
	int c;
	size_t i = 0, j = 0;
	for (; j < len; j++)
		if ((c = tr_char(tr, in[j])) >= 0)
			out[i++] = c;
	return i;
}

tr_state_t *tr_new(void)
{
	return calloc(1, sizeof(tr_state_t));
}

void tr_delete(tr_state_t * st)
{
	free(st);
}

/** @file       valid.c
 *  @brief      validate a list against a type format string, see the
 *              "liblisp.h" header or the code for the format options.
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com 
 *  
 *  @todo    Grouped format specifiers should be treated as an "or", or they
 *           should be be treated as an "and", with the length being calculated
 *           correctly, which it is not.
 *  @warning The number of arguments in the string and the number of arguments
 *           passed into the validation string must be the same, this is the
 *           responsibility of the user of these functions
 **/
 
#include "liblisp.h"
#include "private.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define LISP_VALIDATE_ARGS_XLIST\
        X('s', "symbol",            is_sym(x))\
        X('d', "integer",           is_int(x))\
        X('c', "cons",              is_cons(x))\
        X('L', "cons-or-nil",       is_cons(x) || is_nil(x))\
        X('p', "procedure",         is_proc(x))\
        X('r', "subroutine",        is_subr(x))\
        X('S', "string",            is_str(x))\
        X('P', "io-port",           is_io(x))\
        X('h', "hash",              is_hash(x))\
        X('F', "f-expr",            is_fproc(x))\
        X('f', "float",             is_floating(x))\
        X('u', "user-defined",      is_userdef(x))\
        X('b', "t-or-nil",          is_nil(x) || x == gsym_tee())\
        X('i', "input-port",        is_in(x))\
        X('o', "output-port",       is_out(x))\
        X('Z', "symbol-or-string",  is_asciiz(x))\
        X('a', "integer-or-float",  is_arith(x))\
        X('x', "function",          is_func(x))\
        X('I', "input-port-or-string", is_in(x) || is_str(x))\
        X('l', "defined-procedure", is_proc(x) || is_fproc(x))\
        X('C', "symbol-string-or-integer", is_asciiz(x) || is_int(x))\
        X('A', "any-expression",    1)

static int print_type_string(lisp_t *l, const char *msg, unsigned len, const char *fmt, lisp_cell_t *args)
{
        const char *s, *head = fmt;
        char c;
        io_t *e = lisp_get_logging(l);
        msg = msg ? msg : "";
        lisp_printf(l, e, 0, 
                "\n(%Berror%t\n %y'validation\n %r\"%s\"\n%t '(%yexpected-length %r%d%t)\n '(%yexpected-arguments%t ", 
                msg, (intptr_t)len); 
        while((c = *fmt++)) {
                s = "";
                switch(c) {
                case ' ': continue;
#define X(CHAR, STRING, ACTION) case (CHAR): s = (STRING); break;
                LISP_VALIDATE_ARGS_XLIST
#undef X
                default: LISP_RECOVER(l, "\"invalid format string\" \"%s\" %S))", head, args);
                }
                lisp_printf(l, e, 0, "%y'%s%t", s);
                if(*fmt) io_putc(' ', e);
        }
        return lisp_printf(l, e, 1, ") %S)\n", args);
}

size_t lisp_validate_arg_count(const char *fmt)
{
	size_t i = 0;
	if (!fmt)
		return 0;
	for (; *fmt; i++) {
		while (*fmt && isspace(*fmt++)) ;
		while (*fmt && !isspace(*fmt++)) ;
	}
	return i;
}

int lisp_validate_cell(lisp_t * l, lisp_cell_t * x, lisp_cell_t * args, int recover)
{
	char *fmt, *msg;
	lisp_cell_t *ds;
	assert(x && is_func(x));
	ds = get_func_docstring(x);
	msg = get_str(ds);
	msg = msg ? msg : "";
	fmt = get_func_format(x);
	if (!fmt)
		return 1;	/*as there is no validation string, its up to the function */
	return lisp_validate_args(l, msg, get_length(x), fmt, args, recover);
}

int lisp_validate_args(lisp_t * l, const char *msg, unsigned len, const char *fmt, lisp_cell_t * args, int recover)
{
	assert(l && fmt && args && msg);
	int v = 1;
	char c;
	const char *fmt_head;
	lisp_cell_t *args_head, *x;
	assert(l && fmt && args);
	args_head = args;
	fmt_head = fmt;
	if (!lisp_check_length(args, len))
		goto fail;
	while ((c = *fmt++)) {
		if (is_nil(args) || !v || is_closed(car(args)))
			goto fail;
		v = 0;
		x = car(args);
		switch (c) {
		case ' ':
			v = 1;
			continue;
#define X(CHAR, STRING, ACTION) case (CHAR): v = ACTION; break;
			LISP_VALIDATE_ARGS_XLIST
#undef X
		default:
			LISP_RECOVER(l, "\"%s\"", "invalid validation format");
		}
		args = cdr(args);
	}
	if (!v)
		goto fail;
	return 1;
 fail:	
        print_type_string(l, msg, len, fmt_head, args_head);
	if (recover)
		lisp_throw(l, 1);
	return 0;
}

