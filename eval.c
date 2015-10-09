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
                HALT(l, "%s", "out of memory");
        if(!(node = calloc(1, sizeof(gc_list))))
                HALT(l, "%s", "out of memory");
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

intptr_t intval(cell *x)      { return !x ? 0 : (intptr_t)(x->p[0].v); }
int  is_nil(cell *x)          { assert(x); return x == gsym_nil(); }
int  is_int(cell *x)          { assert(x); return x->type == INTEGER; }
int  is_floatval(cell *x)     { assert(x); return x->type == FLOAT; }
int  is_io(cell *x)           { assert(x); return x->type == IO && !x->close; }
int  is_cons(cell *x)         { assert(x); return x->type == CONS; }
int  is_proc(cell *x)         { assert(x); return x->type == PROC; }
int  is_fproc(cell *x)        { assert(x); return x->type == FPROC; }
int  is_str(cell *x)          { assert(x); return x->type == STRING; }
int  is_sym(cell *x)          { assert(x); return x->type == SYMBOL; }
int  is_subr(cell *x)         { assert(x); return x->type == SUBR; }
int  is_hash(cell *x)         { assert(x); return x->type == HASH; }
int  is_userdef(cell *x)      { assert(x); return x->type == USERDEF && !x->close; }
int  is_usertype(cell *x, int type) { assert(x);
        return x->type == USERDEF && user_type(x) == type && !x->close;
}
int  is_asciiz(cell *x)       { assert(x); return is_str(x) || is_sym(x); }
int  is_arith(cell *x)        { assert(x); return is_int(x) || is_floatval(x); }
int  is_func(cell *x)         { assert(x); return is_proc(x) || is_fproc(x) || is_subr(x); }
int  is_closed(cell *x)       { assert(x); return x->close; }

static cell *mk_asciiz(lisp *l, char *s, lisp_type type) { 
        assert(l && s && (type == STRING || type == SYMBOL));
        cell *x = mk(l, type, 1, (cell *)s); 
        if(!x) return NULL;
        x->len = strlen(s);
        return x;
}
static cell *mk_sym(lisp *l, char *s) { return mk_asciiz(l, s, SYMBOL); }
cell *mk_int(lisp *l, intptr_t d) { assert(l); return mk(l, INTEGER, 1, (cell*) d); }
cell *mk_io(lisp *l, io *x)    { assert(l && x); return mk(l, IO, 1, (cell*)x); }
cell *mk_subr(lisp *l, subr p) { assert(l && p); return mk(l, SUBR, 3, p, NULL, NULL); }

cell *mk_subr_long(lisp *l, subr p, const char *fmt, const char *doc) { assert(l && p); 
        cell *t;
        t = mk(l, SUBR, 3, p); 
        if(fmt) t->len = validate_arg_count(fmt);
        t->p[1].v = (void*)fmt;
        t->p[2].v = (void*)doc;
        return t;
}

cell *mk_proc(lisp *l, cell *x, cell *y, cell *z)  { assert(x); return mk(l, PROC,  3, x, y, z); }
cell *mk_fproc(lisp *l, cell *x, cell *y, cell *z) { assert(x); return mk(l, FPROC, 3, x, y, z); }
cell *mk_float(lisp *l, lfloat f) { assert(l); return mk(l, FLOAT, 1, f); }
cell *mk_str(lisp *l, char *s) { return mk_asciiz(l, s, STRING); }
cell *mk_hash(lisp *l, hashtable *h) { return mk(l, HASH, 1, (cell *)h); }
cell *mk_user(lisp *l, void *x, intptr_t type) { assert(l && x);
        cell *ret = mk(l, USERDEF, 2, x);
        ret->p[1].v = (void*) type;
        return ret;
}

subr  subrval(cell *x)         { assert(x && is_subr(x)); return x->p[0].prim; }
char *subrformat(cell *x)      { assert(x && is_subr(x)); return (char *) x->p[1].v; }
char *subrdocstr(cell *x)      { assert(x && is_subr(x)); return (char *) x->p[2].v; }
cell *proc_args(cell *x)       { assert(x && (is_proc(x) || is_fproc(x))); return x->p[0].v; }
cell *proc_code(cell *x)       { assert(x && (is_proc(x) || is_fproc(x))); return x->p[1].v; }
cell *proc_env(cell *x)        { assert(x && (is_proc(x) || is_fproc(x))); return x->p[2].v; }
char *proc_format(cell *x)     { assert(x && (is_proc(x) || is_fproc(x))); return (char *) x->p[3].v; }
char *proc_docstr(cell *x)     { assert(x && (is_proc(x) || is_fproc(x))); return (char *) x->p[4].v; }
cell *car(cell *x)             { assert(x && is_cons(x)); return x->p[0].v; }
cell *cdr(cell *x)             { assert(x && is_cons(x)); return x->p[1].v; }
io*  ioval(cell *x)            { assert(x /*&& x->type == IO*/); return (io*)(x->p[0].v); }
void set_car(cell *x, cell *y) { assert(x && y); x->p[0].v = y; }
void set_cdr(cell *x, cell *y) { assert(x && y); x->p[1].v = y; }
char *symval(cell *x)          { assert(x); return (char *)(x->p[0].v); }
char *strval(cell *x)          { assert(x && is_asciiz(x)); return (char *)(x->p[0].v); }
void *userval(cell *x)         { assert(x && is_userdef(x)); return (void *)(x->p[0].v); }
intptr_t user_type(cell *x)    { assert(x && is_userdef(x)); return (intptr_t)x->p[1].v; }
hashtable *hashval(cell *x)    { assert(x && is_hash(x)); return (hashtable *)(x->p[0].v); }
int  cklen(cell *x, size_t expect) { assert(x); return (x->len) == expect; }
lfloat floatval(cell *x)       { assert(x && is_floatval(x)); return x->p[0].f; }
intptr_t a2i_val(cell *x) { assert(x && is_arith(x));
       return is_int(x) ? intval(x) : (intptr_t) floatval(x);
}

lfloat a2f_val(cell *x) { assert(x && is_arith(x));
       return is_floatval(x) ? floatval(x) : (lfloat) intval(x);
}

int newuserdef(lisp *l, ud_free f, ud_mark m, ud_equal e, ud_print p) {
        if(l->userdef_used >= MAX_USER_TYPES) return -1;
        l->ufuncs[l->userdef_used].free  = f;
        l->ufuncs[l->userdef_used].mark  = m;
        l->ufuncs[l->userdef_used].equal = e;
        l->ufuncs[l->userdef_used].print = p;
        return l->userdef_used++;
}

int is_in(cell *x) { assert(x);
        if(!x || !is_io(x) 
              || (ioval(x)->type != FIN && ioval(x)->type != SIN)) return 0;
        return 1;
}

int is_out(cell *x) { assert(x);
        if(!x || !is_io(x) 
              ||    (ioval(x)->type != SOUT 
                 &&  ioval(x)->type != FOUT 
                 &&  ioval(x)->type != NULLOUT)) return 0;
        return 1;
}

cell *extend(lisp *l, cell *env, cell *sym, cell *val) { 
        return cons(l, cons(l, sym, val), env); 
}

cell *intern(lisp *l, char *name) { assert(l && name);
        cell *op = hash_lookup(hashval(l->all_symbols), name);
        if(op) return op;
        op = mk_sym(l, name);
        hash_insert(hashval(l->all_symbols), name, op);
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
        set_cdr(l->top_env, cons(l, cons(l, sym, val), cdr(l->top_env)));
        return val;
}

cell *assoc(cell *key, cell *alist) { assert(key && alist);
        /*@note assoc is the main offender when it comes to slowing
         *      the interpreter down, speeding this up will greatly help*/
        cell *lookup;
        for(;!is_nil(alist); alist = cdr(alist))
                if(is_cons(car(alist))) { /*normal assoc*/
                        if(intval(CAAR(alist)) == intval(key)) 
                                return car(alist);
                } else if(is_hash(car(alist)) && is_asciiz(key)) { /*assoc extended with hashes*/
                        if((lookup = hash_lookup(hashval(car(alist)), strval(key))))
                                return lookup; 
                }
        return gsym_nil();
}

/******************************** evaluator ***********************************/

static cell *evlis(lisp *l, unsigned depth, cell *exps, cell *env);
cell *eval(lisp *l, unsigned depth, cell *exp, cell *env) { assert(l);
        size_t gc_stack_save = l->gc_stack_used;
        cell *tmp, *first, *proc, *vals = gsym_nil();
        if(depth > l->max_depth)
                RECOVER(l, "'recursion-depth-reached %d", depth);
        gc_add(l, exp);
        gc_add(l, env);
        tail:
        if(!exp || !env) return NULL;
        if(l->trace) lisp_printf(l, l->efp, 1, "(%ytrace%t %S)\n", exp); 
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
                if(is_nil(tmp = assoc(exp, env)))
                        RECOVER(l, "\"unbound symbol\" '%s", symval(exp));
                return cdr(tmp);
        case CONS:
                first = car(exp);
                exp   = cdr(exp);
                if(first == l->iif) {
                        VALIDATE(l, 3, "A A A", exp, 1);
                        exp = !is_nil(eval(l, depth+1, car(exp), env)) ?
                                CADR(exp) :
                                CADDR(exp);
                        goto tail;
                }
                if(first == l->lambda) {
                        if(exp->len < 2)
                                RECOVER(l, "'lambda \"argc < 2 in %S\"", exp);
                        l->gc_stack_used = gc_stack_save;
                        tmp = mk_proc(l, car(exp), cdr(exp), env);
                        return gc_add(l, tmp);
                }
                if(first == l->flambda) {
                        if(exp->len < 2)
                                RECOVER(l, "'flambda \"argc < 2 in %S\"", exp);
                        if(!cklen(car(exp), 1)) 
                                RECOVER(l, "'flambda \"only one argument allowed %S\"", exp);
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
                if(first == l->env)   return env;
                if(first == l->quote) return car(exp);
                if(first == l->ret)   return cons(l, l->ret, 
                                               cons(l, 
                                                 eval(l, depth+1, car(exp), env), l->nil));
                if(first == l->define) {
                        VALIDATE(l, 2, "s A", exp, 1);
                        l->gc_stack_used = gc_stack_save;
                        return gc_add(l, extend_top(l, car(exp),
                              eval(l, depth+1, CADR(exp), env)));
                }
                if(first == l->set) {
                        cell *pair, *newval;
                        VALIDATE(l, 2, "s A", exp, 1);
                        if(is_nil(pair = assoc(car(exp), env)))
                                RECOVER(l, "'set! \"undefined variable\" '%S", exp);
                        newval = eval(l, depth+1, CADR(exp), env);
                        set_cdr(pair, newval);
                        return newval;
                }
                if(first == l->let) {
                        cell *r = NULL, *s = NULL;
                        if(exp->len < 2)
                                RECOVER(l, "'let* \"argc < 2 in %S\"", exp);
                        tmp = exp;
                        for(; !is_nil(cdr(exp)); exp = cdr(exp)) {
                                if(!is_cons(car(exp)) || !cklen(car(exp), 2))
                                   RECOVER(l, "'let* \"expected list of length 2: '%S '%S\"",
                                                   car(exp), tmp);
                                s = env = extend(l, env, CAAR(exp), l->nil);
                                r = env = extend(l, env, CAAR(exp),
                                        eval(l, depth + 1, CADAR(exp), env));
                                set_cdr(car(s), CDAR(r));
                        }
                        return eval(l, depth+1, car(exp), env);
                }
                if(first == l->progn) { /**@todo Add looping constructs here*/
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
                        RECOVER(l, "\"not a procedure\" '%S", first);
                }
                if(is_subr(proc)) {
                        l->gc_stack_used = gc_stack_save;
                        gc_add(l, proc);
                        gc_add(l, vals);
                        l->cur_depth = depth; /*tucked away for subr use*/
                        if(subrformat(proc))
                                VALIDATE(l, proc->len, subrformat(proc), vals, 1);
                        return (*subrval(proc))(l, vals);
                }
                if(is_proc(proc) || is_fproc(proc)) {
                        if(proc_args(proc)->len != vals->len)
                                RECOVER(l, "'proc \"expected %S\" '%S", 
                                                proc_args(proc), vals);
                        if(proc_args(proc)->len)
                                env = multiple_extend(l, 
                                        l->dynamic ? env : proc_env(proc), 
                                        proc_args(proc), vals);
                        exp = cons(l, l->progn, proc_code(proc));
                        goto tail;
                }
                RECOVER(l, "\"not a procedure\" '%S", first);
        case INVALID: 
        default:     HALT(l, "%s", "internal inconsistency: unknown type");
        }
        HALT(l, "%s", "internal inconsistency: reached the unreachable");
        return exp; 
}

static cell *evlis(lisp *l, unsigned depth, cell *exps, cell *env) { /**< evaluate a list*/
        size_t i;
        cell *op, *head, *start = exps;
        assert(l && exps && env);
        if(is_nil(exps)) return l->nil;
        op = car(exps);
        exps = cdr(exps);
        head = op = cons(l, eval(l, depth+1, op, env), l->nil);
        if(!is_nil(exps) && !is_cons(exps)) /*is there a better way of doing this?*/
                RECOVER(l, "\"evlis cannot eval dotted pairs\" '%S", start);
        for(i = 1; !is_nil(exps); exps = cdr(exps), op = cdr(op), i++) {
                if(!is_nil(cdr(exps)) && !is_cons(cdr(exps)))
                        RECOVER(l, "\"evlis cannot eval dotted pairs\"'%S", start);
                set_cdr(op, cons(l, eval(l, depth+1, car(exps), env), l->nil));
        }
        head->len = i;
        return head;
}

