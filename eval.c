/** @file       eval.c
 *  @brief      The evaluator for the lisp interpreter
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *
 *  This is the main evaluator and associated function, the built in
 *  subroutines for the interpreter are defined elsewhere. 
 *  
 *  @note constants could be added by performing checks in "define" and
 *        "set!" against a new flag in a lisp cell, and they would refuse
 *        to operate by redefining or modifying what the symbols eval
 *        to. The same could be done for operations that mutate data.
 *  **/
#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static cell *mk(lisp *l, lisp_type type, size_t count, ...) 
{ /**@brief make new lisp cells and perform garbage bookkeeping/collection*/
        assert(l && type != INVALID && count);
        cell *ret;
        gc_list *node; /**< new node in linked list of all allocations*/
        va_list ap;
        size_t i;

        if(l->gc_collectp++ > COLLECTION_POINT) /*set to 1 for testing*/
                gc_mark_and_sweep(l);

        va_start(ap, count);
        if(!(ret = calloc(1, sizeof(cell) + (count - 1) * sizeof(cell_data))))
                HALT(l, "\"%s\"", "out of memory");
        if(!(node = calloc(1, sizeof(gc_list))))
                HALT(l, "\"%s\"", "out of memory");
        ret->type = type;
        for(i = 0; i < count; i++) 
                if     (FLOAT == type) ret->p[i].f    = va_arg(ap, double);
                else if(SUBR == type)  ret->p[i].prim = va_arg(ap, subr);
                else                   ret->p[i].v    = va_arg(ap, void*);
        va_end(ap);
        node->ref = ret;
        node->next = l->gc_head;
        l->gc_head = node;
        gc_add(l, ret);
        return ret;
}

cell* cons(lisp *l, cell *x, cell *y) {
        cell *z = mk(l, CONS, 2, x, y);
        assert(l /*&& x && y*/);
        if(!z || !x || !y) return NULL;
        if(is_cons(y)) z->len = y->len + 1;
        if(is_nil(y)) z->len++;
        return z;
}

cell *car(cell *x)              { assert(x && is_cons(x)); return x->p[0].v; }
cell *cdr(cell *x)              { assert(x && is_cons(x)); return x->p[1].v; }
void set_car(cell *x, cell *y)  { assert(x && is_cons(x) && y); x->p[0].v = y; }
void set_cdr(cell *x, cell *y)  { assert(x && is_cons(x) && y); x->p[1].v = y; }
void close_cell(cell *x)        { assert(x); x->close = 1; }
int  cklen(cell *x, size_t expect) { assert(x); return x->len == expect; }
int  is_nil(cell *x)          { assert(x); return x == gsym_nil(); }
int  is_int(cell *x)          { assert(x); return x->type == INTEGER; }
int  is_floating(cell *x)     { assert(x); return x->type == FLOAT; }
int  is_io(cell *x)           { assert(x); return x->type == IO && !x->close; }
int  is_cons(cell *x)         { assert(x); return x->type == CONS; }
int  is_proper_cons(cell *x)  { assert(x); return is_cons(x) && (is_nil(cdr(x)) || is_cons(cdr(x))); }
int  is_proc(cell *x)         { assert(x); return x->type == PROC; }
int  is_fproc(cell *x)        { assert(x); return x->type == FPROC; }
int  is_str(cell *x)          { assert(x); return x->type == STRING; }
int  is_sym(cell *x)          { assert(x); return x->type == SYMBOL; }
int  is_subr(cell *x)         { assert(x); return x->type == SUBR; }
int  is_hash(cell *x)         { assert(x); return x->type == HASH; }
int  is_userdef(cell *x)      { assert(x); return x->type == USERDEF && !x->close; }
int  is_usertype(cell *x, int type) { assert(x);
        return x->type == USERDEF && get_user_type(x) == type && !x->close;
}
int  is_asciiz(cell *x)       { assert(x); return is_str(x) || is_sym(x); }
int  is_arith(cell *x)        { assert(x); return is_int(x) || is_floating(x); }
int  is_func(cell *x)         { assert(x); return is_proc(x) || is_fproc(x) || is_subr(x); }
int  is_closed(cell *x)       { assert(x); return x->close; }

static cell *mk_asciiz(lisp *l, char *s, lisp_type type) {
        size_t slen;
        assert(l && s && (type == STRING || type == SYMBOL));
        cell *x = mk(l, type, 1, (cell *)s); 
        if(!x) return NULL;
        slen = strlen(s);
        assert((BITS_IN_LENGTH >= 32) && slen < 0xFFFFFFFFu);
        x->len = slen;
        return x;
}
static cell *mk_sym(lisp *l, char *s) { return mk_asciiz(l, s, SYMBOL); }

cell *mk_list(lisp *l, cell *x, ...) { assert(x);
        size_t i;
        cell *head, *op, *next;
        va_list ap;
        head = op = cons(l, x, gsym_nil());
        va_start(ap, x);
        for(i = 1;(next = va_arg(ap, cell*)); op = cdr(op), i++)
                set_cdr(op, cons(l, next, gsym_nil()));
        va_end(ap);
        head->len = i;
        return head;
}

cell *mk_int(lisp *l, intptr_t d) { assert(l); return mk(l, INTEGER, 1, (cell*) d); }
cell *mk_io(lisp *l, io *x)    { assert(l && x); return mk(l, IO, 1, (cell*)x); }

cell *mk_subr(lisp *l, subr p, const char *fmt, const char *doc) { assert(l && p); 
        cell *t;
        t = mk(l, SUBR, 3, p); 
        if(fmt) { 
                size_t tlen = validate_arg_count(fmt); 
                assert((BITS_IN_LENGTH >= 32) && tlen < 0xFFFFFFFFu);
                t->len = tlen;
        }
        t->p[1].v = (void*)fmt;
        t->p[2].v = (void*)mk_str(l, lstrdup(doc ? doc : ""));
        return t;
}

cell *mk_proc(lisp *l, cell *args, cell *code, cell *env, cell *doc)  { assert(l && args && code && env); 
        return mk(l, PROC,  5, args, code, env, NULL, doc); 
}

cell *mk_fproc(lisp *l, cell *args, cell *code, cell *env) { assert(l && args && code && env); 
        cell *ds = l->empty_docstr;
        return mk(l, FPROC, 5, args, code, env, NULL, ds); 
}

cell *mk_float(lisp *l, lfloat f) { assert(l); return mk(l, FLOAT, 1, f); }
cell *mk_str(lisp *l, char *s) { return mk_asciiz(l, s, STRING); }
cell *mk_hash(lisp *l, hashtable *h) { return mk(l, HASH, 1, (cell *)h); }
cell *mk_user(lisp *l, void *x, intptr_t type) { 
        assert(l && x && type >= 0 && type < l->user_defined_types_used);
        cell *ret = mk(l, USERDEF, 2, x);
        ret->p[1].v = (void*) type;
        return ret;
}

unsigned get_length(cell *x)       { assert(x); return x->len; }
void *get_raw(cell *x)             { assert(x); return x->p[0].v; }
intptr_t get_int(cell *x)          { return !x ? 0 : (intptr_t)(x->p[0].v); }
subr  get_subr(cell *x)            { assert(x && is_subr(x)); return x->p[0].prim; }
/**@note it might make sense to merge get_subr_docstring and get_proc_docstring
 *       as well as get_subr_format and get_proc_format */
char *get_subr_format(cell *x)     { assert(x && is_subr(x)); return (char *) x->p[1].v; }
cell *get_subr_docstring(cell *x)  { assert(x && is_subr(x)); return x->p[2].v; }
cell *get_proc_args(cell *x)       { assert(x && (is_proc(x) || is_fproc(x))); return x->p[0].v; }
cell *get_proc_code(cell *x)       { assert(x && (is_proc(x) || is_fproc(x))); return x->p[1].v; }
cell *get_proc_env(cell *x)        { assert(x && (is_proc(x) || is_fproc(x))); return x->p[2].v; }
char *get_proc_format(cell *x)     { assert(x && (is_proc(x) || is_fproc(x))); return (char *) x->p[3].v; }
cell *get_proc_docstring(cell *x)  { assert(x && (is_proc(x) || is_fproc(x))); return x->p[4].v; }
io   *get_io(cell *x)           { assert(x && x->type == IO);  return (io*)(x->p[0].v); }
char *get_sym(cell *x)          { assert(x && is_asciiz(x));   return (char *)(x->p[0].v); }
char *get_str(cell *x)          { assert(x && is_asciiz(x));   return (char *)(x->p[0].v); }
void *get_user(cell *x)         { assert(x && x->type == USERDEF); return (void *)(x->p[0].v); }
int   get_user_type(cell *x)    { assert(x && x->type == USERDEF); return (intptr_t)x->p[1].v; }
hashtable *get_hash(cell *x)    { assert(x && is_hash(x));     return (hashtable *)(x->p[0].v); }
lfloat get_float(cell *x)       { assert(x && is_floating(x)); return x->p[0].f; }
intptr_t get_a2i(cell *x) { assert(x && is_arith(x));
       return is_int(x) ? get_int(x) : (intptr_t) get_float(x);
}

lfloat get_a2f(cell *x) { assert(x && is_arith(x));
       return is_floating(x) ? get_float(x) : (lfloat) get_int(x);
}

int is_in(cell *x)  { assert(x); return (x && is_io(x) && io_is_in(get_io(x)))  ? 1 : 0; }
int is_out(cell *x) { assert(x); return (x && is_io(x) && io_is_out(get_io(x))) ? 1 : 0; }

int new_user_defined_type(lisp *l, ud_free f, ud_mark m, ud_equal e, ud_print p) {
        if(l->user_defined_types_used >= MAX_USER_TYPES) return -1;
        l->ufuncs[l->user_defined_types_used].free  = f;
        l->ufuncs[l->user_defined_types_used].mark  = m;
        l->ufuncs[l->user_defined_types_used].equal = e;
        l->ufuncs[l->user_defined_types_used].print = p;
        return l->user_defined_types_used++;
}

cell *extend(lisp *l, cell *env, cell *sym, cell *val) { 
        return cons(l, cons(l, sym, val), env); 
}

cell *intern(lisp *l, char *name) { assert(l && name);
        cell *op = hash_lookup(get_hash(l->all_symbols), name);
        if(op) return op;
        op = mk_sym(l, name);
        hash_insert(get_hash(l->all_symbols), name, op);
        return op;
}

/***************************** environment ************************************/

static cell *multiple_extend(lisp *l, cell *env, cell *syms, cell *vals) {
        assert(l && env && syms && vals);
        for(;!is_nil(syms); syms = cdr(syms), vals = cdr(vals))
                env = extend(l, env, car(syms), car(vals));
        return env;
}

cell *extend_top(lisp *l, cell *sym, cell *val) { assert(l && sym && val);
        if(hash_insert(get_hash(l->top_hash), get_str(sym), cons(l, sym, val)) < 0)
                HALT(l, "\"%s\"", "out of memory");
        return val;
}

cell *assoc(cell *key, cell *alist) { assert(key && alist);
        /*@note assoc is the main offender when it comes to slowing
         *      the interpreter down, speeding this up will greatly help*/
        cell *lookup;
        for(;!is_nil(alist); alist = cdr(alist))
                if(is_cons(car(alist))) { /*normal assoc*/
                        if(get_int(CAAR(alist)) == get_int(key)) 
                                return car(alist);
                } else if(is_hash(car(alist)) && is_asciiz(key)) { /*assoc extended with hashes*/
                        if((lookup = hash_lookup(get_hash(car(alist)), get_str(key))))
                                return lookup; 
                }
        return gsym_nil();
}

/******************************** evaluator ***********************************/

/** @brief "Compile" an expression, that is, perform optimizations
 *         and bind as many variables as possible. This function
 *         is a work in progress.
 *  @note Things that can be done here; partial argument checking,
 *        optimizing called procedures, constant folding and
 *        inlining small procedures. This function could also
 *        use set_car to replace the current list to save memory.
 **/        
static cell *compiler(lisp *l, unsigned depth, cell *exp, cell *env) {
        size_t i;
        cell *head, *op, *tmp, *code = gsym_nil();
        if(depth > MAX_RECURSION_DEPTH)
                RECOVER(l, "%y'recursion-depth-reached%t %d", depth);
        if(is_sym(car(exp)) && !is_nil(tmp = assoc(car(exp), env))) 
                op = cdr(tmp);
        else if (is_cons(car(exp)))
                op = compiler(l, depth + 1, car(exp), env);
        else 
                op = car(exp);
        head = op = cons(l, op, gsym_nil());
        exp = cdr(exp);
        for(i = 1; !is_nil(exp); exp = cdr(exp), op = cdr(op), i++) {
                code = car(exp);
                if(is_sym(car(exp)) && !is_nil(tmp = assoc(car(exp), env)))
                        code = cdr(tmp);
                if(is_cons(car(exp)))
                        code = compiler(l, depth+1, car(exp), env);
                set_cdr(op, cons(l, code, gsym_nil()));
        }
        head->len = i;
        for(op = cdr(head); !is_nil(op); op = cdr(op))
                op->len = --i;
        return head;
}

static cell *evlis(lisp *l, unsigned depth, cell *exps, cell *env);
cell *eval(lisp *l, unsigned depth, cell *exp, cell *env) { assert(l);
        size_t gc_stack_save = l->gc_stack_used;
        cell *tmp, *first, *proc, *vals = gsym_nil();
        if(depth > MAX_RECURSION_DEPTH)
                RECOVER(l, "%y'recursion-depth-reached%t %d", depth);
        gc_add(l, exp);
        gc_add(l, env);
        tail:
        if(!exp || !env) return NULL;
        if(l->trace_on) lisp_printf(l, lisp_get_logging(l), 1, "(%ytrace%t %S)\n", exp); 
        if(is_nil(exp)) return gsym_nil();
        if(l->sig) {
                l->sig = 0;
                lisp_throw(l, 1);
        }

        switch(exp->type) {
        case INTEGER: case SUBR: case PROC:  case STRING: case FLOAT:
        case IO:      case HASH: case FPROC: case USERDEF:
                return exp; /*self evaluating types*/
        case SYMBOL:
                /* checks could be added here so special forms are not looked
                 * up, but only if this improves the speed of things*/
                if(is_nil(tmp = assoc(exp, env)))
                        RECOVER(l, "%r\"unbound symbol\"%t\n '%s", get_sym(exp));
                return cdr(tmp);
        case CONS:
                first = car(exp);
                exp   = cdr(exp);
                if(!is_nil(exp) && !is_proper_cons(exp))
                        RECOVER(l, "%y'evaluation\n %r\"cannot eval dotted pair\"%t\n '%S", exp);
                if(is_cons(first))
                        first = eval(l, depth+1, first, env);
                if(first == l->iif) {
                        VALIDATE(l, "if", 3, "A A A", exp, 1);
                        exp = !is_nil(eval(l, depth+1, car(exp), env)) ?
                                CADR(exp) :
                                CADDR(exp);
                        goto tail;
                }
                if(first == l->lambda) {
                        cell *doc;
                        if(get_length(exp) < 2)
                                RECOVER(l, "%y'lambda\n %r\"argc < 2\"%t\n '%S\"", exp);
                        if(!is_nil(car(exp)) && is_asciiz(car(exp))) { /*have docstring*/
                                doc = car(exp);
                                exp = cdr(exp);
                        } else {
                                doc = l->empty_docstr;
                        }
                        if(!is_nil(car(exp)) && !is_cons(car(exp)))
                                RECOVER(l, "'lambda\n \"not an argument list (or nil)\"\n '%S", exp);
                        for(tmp = car(exp); !is_nil(tmp); tmp = cdr(tmp)) 
                                if(!is_sym(car(tmp)) || !is_proper_cons(tmp))
                                        RECOVER(l, "%y'lambda\n %r\"expected only symbols (or nil) as arguments\"%t\n '%S", exp);
                        l->gc_stack_used = gc_stack_save;
                        tmp = mk_proc(l, car(exp), cdr(exp), env, doc);
                        return gc_add(l, tmp);
                }
                if(first == l->flambda) {
                        if(exp->len < 2)
                                RECOVER(l, "%y'flambda\n %r\"argc < 2\"%t\n '%S", exp);
                        if(!cklen(car(exp), 1) || !is_sym(car(car(exp)))) 
                                RECOVER(l, "%y'flambda\n %r\"only one symbol argument allowed\"%t\n '%S", exp);
                        l->gc_stack_used = gc_stack_save;
                        return gc_add(l, mk_fproc(l, car(exp), cdr(exp), env));
                }
                if(first == l->cond) {
                        if(cklen(exp, 0)) return l->nil;
                        for(tmp = l->nil; is_nil(tmp) && !is_nil(exp); exp=cdr(exp)) {
                                if(!is_cons(car(exp))) return l->nil;
                                tmp = eval(l, depth+1, CAAR(exp), env);
                                if(!is_nil(tmp)) {
                                        exp = CADAR(exp);
                                        goto tail;
                                }
                        }
                        return l->nil;
                }
                if(first == l->quote) return car(exp);
                if(first == l->ret)   return cons(l, l->ret, 
                                               cons(l, 
                                                 eval(l, depth+1, car(exp), env), l->nil));
                if(first == l->define) {
                        VALIDATE(l, "define", 2, "s A", exp, 1);
                        l->gc_stack_used = gc_stack_save;
                        return gc_add(l, extend_top(l, car(exp),
                              eval(l, depth+1, CADR(exp), env)));
                }
                if(first == l->set) {
                        cell *pair, *newval;
                        VALIDATE(l, "set!", 2, "s A", exp, 1);
                        if(is_nil(pair = assoc(car(exp), env)))
                                RECOVER(l, "%y'set!\n %r\"undefined variable\"%t\n '%S", exp);
                        newval = eval(l, depth+1, CADR(exp), env);
                        set_cdr(pair, newval);
                        return newval;
                }
                if(first == l->compile) {
                        cell *doc;
                        VALIDATE(l, "compile", 3, "Z L c", exp, 1);
                        doc = car(exp);
                        for(tmp = CADR(exp); !is_nil(tmp); tmp = cdr(tmp)) 
                                if(!is_sym(car(tmp)) || !is_proper_cons(tmp))
                                        RECOVER(l, "%y'lambda\n %r\"expected only symbols (or nil) as arguments\"%t\n %S", exp);
                        tmp = compiler(l, depth+1, CADDR(exp), env);
                        return mk_proc(l, CADR(exp), cons(l, tmp, gsym_nil()), env, doc);
                }
                if(first == l->let) {
                        cell *r = NULL, *s = NULL;
                        if(exp->len < 2)
                                  RECOVER(l, "%y'let\n %r\"argc < 2\"%t\n '%S", exp);
                        tmp = exp;
                        for(; !is_nil(cdr(exp)); exp = cdr(exp)) {
                                if(!is_cons(car(exp)) || !cklen(car(exp), 2))
                                   RECOVER(l, "%y'let\n %r\"expected list of length 2\"%t\n '%S\n '%S",
                                                   car(exp), tmp);
                                s = env = extend(l, env, CAAR(exp), l->nil);
                                r = env = extend(l, env, CAAR(exp),
                                        eval(l, depth + 1, CADAR(exp), env));
                                set_cdr(car(s), CDAR(r));
                        }
                        return eval(l, depth+1, car(exp), env);
                }
                /**@bug  loop/return and tail call optimization do not mix
                 * @todo improve looping constructs */
                if(first == l->progn) { 
                        cell *head = exp, *tmp;
                        if(is_nil(exp)) return l->nil;
                begin:  for(exp = head;;) {
                                if(is_nil(cdr(exp))) {
                                      exp = car(exp);
                                      goto tail;
                                }
                                l->gc_stack_used = gc_stack_save;
                                tmp = eval(l, depth + 1, car(exp), env);
                                if(tmp == l->loop) goto begin;
                                if(is_cons(tmp) && car(tmp) == l->ret)
                                        return CADR(tmp);
                                exp = cdr(exp);
                        }
                }

                proc = eval(l, depth + 1, first, env);
                if(is_proc(proc) || is_subr(proc)) { /*eval their args*/
                        vals = evlis(l, depth + 1, exp, env);
                } else if(is_fproc(proc)) { /*f-expr do not eval their args*/
                        vals = cons(l, exp, l->nil);
                } else { 
                        RECOVER(l, "%r\"not a procedure\"%t\n '%S", first);
                }
                l->cur_depth = depth; /*tucked away for function use*/
                l->cur_env   = env;   /*also tucked away*/
                if(is_subr(proc)) {
                        l->gc_stack_used = gc_stack_save;
                        gc_add(l, proc);
                        gc_add(l, vals);
                        lisp_validate_cell(l, proc, vals, 1);
                        return (*get_subr(proc))(l, vals);
                }
                if(is_proc(proc) || is_fproc(proc)) {
                        if(get_proc_args(proc)->len != vals->len)
                                RECOVER(l, "%y'lambda%t\n '%S\n %y'expected%t\n '%S\n '%S", 
                                                get_proc_docstring(proc), 
                                                get_proc_args(proc), vals);
                        if(get_proc_args(proc)->len)
                                env = multiple_extend(l, 
                                       get_proc_env(proc), 
                                        get_proc_args(proc), vals);
                        exp = cons(l, l->progn, get_proc_code(proc));
                        goto tail;
                }
                RECOVER(l, "%r\"not a procedure\"%t\n '%S", first);
        case INVALID: 
        default:     HALT(l, "%r\"%s\"%t", "internal inconsistency: unknown type");
        }
        HALT(l, "%r\"%s\"%t", "internal inconsistency: reached the unreachable");
        return exp; 
}

static cell *evlis(lisp *l, unsigned depth, cell *exps, cell *env) { /**< evaluate a list*/
        size_t i;
        cell *op, *head, *start = exps;
        assert(l && exps && env);
        if(is_nil(exps)) return gsym_nil();
        op = car(exps);
        exps = cdr(exps);
        head = op = cons(l, eval(l, depth+1, op, env), gsym_nil());
        if(!is_nil(exps) && !is_cons(exps)) /*is there a better way of doing this?*/
                goto fail;
        for(i = 1; !is_nil(exps); exps = cdr(exps), op = cdr(op), i++) {
                if(!is_nil(cdr(exps)) && !is_cons(cdr(exps)))
                        goto fail;
                set_cdr(op, cons(l, eval(l, depth+1, car(exps), env), gsym_nil()));
        }
        head->len = i;
        return head;
fail:   RECOVER(l, "%r\"evlis cannot eval dotted pairs\"%t\n '%S", start);
        return gsym_error();
}

