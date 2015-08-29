/** @file       eval.c
 *  @brief      The evaluator for the lisp interpreter
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com**/
#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

static cell *mk(lisp *l, lisp_type type, size_t count, ...) 
{ /**@brief make new lisp cells and perform garbage bookkeeping/collection*/
        cell *ret;
        gc_list *node; /**< new node in linked list of all allocations*/
        va_list ap;
        size_t i;

        if(l->gc_collectp++ > COLLECTION_POINT) /*Set to 1 for testing*/
                if(l->gc_state == GC_ON) /*only collect when gc is on*/
                        gc_mark_and_sweep(l);

        va_start(ap, count);
        if(!(ret = calloc(1, sizeof(cell) + (count - 1) * sizeof(lisp_union))))
                HALT(l, "%s", "out of memory");
        if(!(node = calloc(1, sizeof(gc_list))))
                HALT(l, "%s", "out of memory");
        ret->type = type;
        for(i = 0; i < count; i++) 
                if     (FLOAT == type)  ret->p[i].f =    va_arg(ap, double);
                else if(SUBR == type)   ret->p[i].prim = va_arg(ap, subr);
                else                    ret->p[i].v =    va_arg(ap, void*);
        va_end(ap);
        node->ref = ret;
        node->next = l->gc_head;
        l->gc_head = node;
        gc_add(l, ret);
        return ret;
}

cell* cons(lisp *l, cell *x, cell *y) {
        cell *z = mk(l, CONS, 2, x, y);
        if(!x || !y || !z) return NULL;
        if(y->type == CONS) z->len = y->len + 1;
        if(isnil(y)) z->len++;
        return z;
}

intptr_t intval(cell *x)     { return !x ? 0 : (intptr_t)(x->p[0].v); }
int  isnil(cell *x)          { return x == mknil(); }
int  isint(cell *x)          { return !x ? 0 : x->type == INTEGER; }
int  isfloat(cell *x)        { return !x ? 0 : x->type == FLOAT; }
int  isio(cell *x)           { return !x ? 0 : x->type == IO && !x->close; }
int  iscons(cell *x)         { return !x ? 0 : x->type == CONS; }
int  isproc(cell *x)         { return !x ? 0 : x->type == PROC; }
int  isfproc(cell *x)        { return !x ? 0 : x->type == FPROC; }
int  isstr(cell *x)          { return !x ? 0 : x->type == STRING; }
int  issym(cell *x)          { return !x ? 0 : x->type == SYMBOL; }
int  issubr(cell *x)         { return !x ? 0 : x->type == SUBR; }
int  ishash(cell *x)         { return !x ? 0 : x->type == HASH; }
int  isuserdef(cell *x)      { return !x ? 0 : x->type == USERDEF; }
int  isusertype(cell *x, int type) {
        if(!x) return 0;
        return x->type == USERDEF && x->userdef == type;
}
int  isasciiz(cell *x)       { return isstr(x) || issym(x); }
int  isarith(cell *x)        { return isint(x) || isfloat(x); }
cell *mkint(lisp *l, intptr_t d) { return mk(l, INTEGER, 1, (cell*) d); }
cell *mkio(lisp *l, io *x)   { return mk(l, IO, 1, (cell*)x); }
cell *mksubr(lisp *l, subr p) { return mk(l, SUBR, 1, p); }
cell *mkproc(lisp *l, cell *x, cell *y, cell *z)  { return mk(l,PROC,3,x,y,z); }
cell *mkfproc(lisp *l, cell *x, cell *y, cell *z) { return mk(l,FPROC,3,x,y,z); }
cell *mkfloat(lisp *l, lfloat f) { return mk(l, FLOAT, 1, f); }
cell *mkuser(lisp *l, void *x, int type) {
        cell *ret = mk(l, USERDEF, 1, x);
        ret->userdef = type;
        return ret;
}

static cell *mkasciiz(lisp *l, char *s, lisp_type type) {  
        assert(s && (type == STRING || type == SYMBOL));
        cell *x = mk(l, type, 1, (cell *)s); 
        if(!x) return NULL;
        x->len = strlen(s);
        return x;
}

cell *mkstr(lisp *l, char *s) { return mkasciiz(l, s, STRING); }
cell *mksym(lisp *l, char *s) { return mkasciiz(l, s, SYMBOL); }
cell *mkhash(lisp *l, hashtable *h) { return mk(l, HASH, 1, (cell *)h); }
subr subrval(cell *x)         { return !x ? NULL : x->p[0].prim; }
cell *procargs(cell *x)       { return !x ? NULL : x->p[0].v; }
cell *proccode(cell *x)       { return !x ? NULL : x->p[1].v; }
cell *procenv(cell *x)        { return !x ? NULL : x->p[2].v; }
cell *car(cell *x)            { return !x ? NULL : x->p[0].v; }
cell *cdr(cell *x)            { return !x ? NULL : x->p[1].v; }
io*  ioval(cell *x)           { return !x ? NULL : (io*)(x->p[0].v); }
void setcar(cell *x, cell *y) { if(!x||!y) return;  x->p[0].v = y; }
void setcdr(cell *x, cell *y) { if(!x||!y) return;  x->p[1].v = y; }
char *symval(cell *x)         { return !x ? NULL : (char *)(x->p[0].v); }
char *strval(cell *x)         { return !x ? NULL : (char *)(x->p[0].v); }
void *userval(cell *x)        { return !x ? NULL : (void *)(x->p[0].v); }
hashtable *hashval(cell *x)   { return !x ? NULL : (hashtable *)(x->p[0].v); }
int  cklen(cell *x, size_t expect)  { return !x ? 0 : (x->len) == expect; }
lfloat floatval(cell *x)      { return !x ? 0.f : x->p[0].f; }

int newuserdef(lisp *l, ud_free f, ud_mark m, ud_equal e, ud_print p) {
        if(l->userdef_used >= MAX_USER_TYPES) return -1;
        l->ufuncs[l->userdef_used].free  = f;
        l->ufuncs[l->userdef_used].mark  = m;
        l->ufuncs[l->userdef_used].equal = e;
        l->ufuncs[l->userdef_used].print = p;
        return l->userdef_used++;
}

int isin(cell *x) {
        if(!x || !isio(x) 
              || (ioval(x)->type != FIN && ioval(x)->type != SIN)) return 0;
        return 1;
}

int isout(cell *x) { 
        if(!x || !isio(x) 
              ||    (ioval(x)->type != SOUT 
                 &&  ioval(x)->type != FOUT 
                 &&  ioval(x)->type != NULLOUT)) return 0;
        return 1;
}

cell *extend(lisp *l, cell *env, cell *sym, cell *val) { 
        return cons(l, cons(l, sym, val), env); 
}

cell *intern(lisp *l, char *name) { assert(name);
        cell *op = hash_lookup(hashval(l->all_symbols), name);
        if(op) return op;
        op = mksym(l, name);
        hash_insert(hashval(l->all_symbols), name, op);
        return op;
}

/***************************** environment ************************************/

static cell *multiple_extend(lisp *l, cell *env, cell *syms, cell *vals) {
        if(!env || !syms || !vals) return NULL;
        for(;!isnil(syms); syms = cdr(syms), vals = cdr(vals))
                env = extend(l, env, car(syms), car(vals));
        return env;
}

cell *extend_top(lisp *l, cell *sym, cell *val) {
        if(!sym || !val) return NULL;
        setcdr(l->top_env, cons(l, cons(l, sym, val), cdr(l->top_env)));
        return val;
}

cell *assoc(cell *key, cell *alist) {
        /*@note assoc is the main offender when it comes to slowing
         *      the interpreter down, speeding this up will greatly help*/
        cell *lookup;
        if(!key || !alist) return NULL;
        for(;!isnil(alist); alist = cdr(alist))
                if(iscons(car(alist))) { /*normal assoc*/
                        if(intval(CAAR(alist)) == intval(key)) 
                                return car(alist);
                } else if(ishash(car(alist)) && isasciiz(key)) { /*assoc extended with hashes*/
                        if((lookup = hash_lookup(hashval(car(alist)), strval(key))))
                                return lookup; 
                }
        return mknil();
}

/******************************** evaluator ***********************************/

static cell *evlis(lisp *l, unsigned depth, cell *exps, cell *env);
cell *eval(lisp *l, unsigned depth, cell *exp, cell *env) { 
        size_t gc_stack_save = l->gc_stack_used;
        cell *tmp, *first, *proc, *vals = l->Nil;
        if(!exp || !env) return NULL;
        if(depth > l->max_depth)
                RECOVER(l, "'recursion-depth-reached %d", depth);
        gc_add(l, exp);
        gc_add(l, env);

        tail:
        if(l->trace != TRACE_OFF) { /*print out expressions for debugging*/
                if(l->trace == TRACE_ALL || (l->trace == TRACE_MARKED && exp->trace))
                        printerf(l, l->efp, 1, "(%ytrace%t %S)\n", exp); 
                else if(l->trace != TRACE_MARKED)
                        HALT(l, "\"invalid trace level\" %d", l->trace);
        }
        if(exp == l->Nil) return l->Nil;
        if(l->sig) {
                l->sig = 0;
                lisp_throw(l, 1);
        }

        switch(exp->type) {
        case INTEGER: case SUBR: case PROC:  case STRING: case FLOAT:
        case IO:      case HASH: case FPROC: case USERDEF:
                return exp; /*self evaluating types*/
        case SYMBOL:   
                if(isnil(tmp = assoc(exp, env)))
                        RECOVER(l, "\"unbound symbol\" '%s", symval(exp));
                return cdr(tmp);
        case CONS:
                first = car(exp);
                exp   = cdr(exp);
                if(first == l->If) {
                        if(!cklen(exp, 3))
                                RECOVER(l, "'if \"argc != 3 in %S\"", exp);
                        exp = !isnil(eval(l, depth+1, car(exp), env)) ?
                                CADR(exp) :
                                CADDR(exp);
                        goto tail;
                }
                if(first == l->Lambda) {
                        if(!cklen(exp, 2))
                                RECOVER(l, "'lambda \"argc != 2 in %S\"", exp);
                        l->gc_stack_used = gc_stack_save;
                        tmp = mkproc(l, car(exp), cdr(exp), env);
                        return gc_add(l, tmp);
                }
                if(first == l->Flambda) {
                        if(!cklen(exp, 2)) 
                                RECOVER(l, "'flambda \"argc != 2 in %S\"", exp);
                        if(!cklen(car(exp), 1)) 
                                RECOVER(l, "'flambda \"only one argument allowed %S\"", exp);
                        l->gc_stack_used = gc_stack_save;
                        return gc_add(l, mkfproc(l, car(exp), cdr(exp), env));
                }
                if(first == l->Cond) {
                        if(cklen(exp, 0)) return l->Nil;
                        for(tmp = l->Nil; isnil(tmp) && !isnil(exp); exp=cdr(exp)) {
                                if(!iscons(car(exp))) return l->Nil;
                                tmp = eval(l, depth+1, CAAR(exp), env);
                                if(!isnil(tmp)) {
                                        exp = CADAR(exp);
                                        goto tail;
                                }
                        }
                        return l->Nil;
                }
                if(first == l->Env)   return env;
                if(first == l->Quote) return car(exp);
                if(first == l->Error) {
                        if(cklen(exp, 1) && isint(car(exp)))
                                lisp_throw(l, intval(car(exp)));
                        if(cklen(exp, 0))
                                lisp_throw(l, -1);
                        RECOVER(l, "'throw \"expected () or (int)\" '%S", exp); 
                }
                if(first == l->Define) {
                        if(!cklen(exp, 2))
                                RECOVER(l, "'define \"argc != 2 in %S\"", exp);
                        l->gc_stack_used = gc_stack_save;
                        return gc_add(l, extend_top(l, car(exp),
                              eval(l, depth+1, CADR(exp), env)));
                }
                if(first == l->Set) {
                        cell *pair, *newval;
                        if(!cklen(exp, 2))
                                RECOVER(l, "'set! \"argc != 2 in %S\"", exp);
                        if(isnil(pair = assoc(car(exp), env)))
                                RECOVER(l, "'set! \"undefined variable\" '%S", exp);
                        newval = eval(l, depth+1, CADR(exp), env);
                        setcdr(pair, newval);
                        return newval;
                }
                if(first == l->LetS || first == l->LetRec) {
                        cell *r = NULL, *s = NULL;
                        if(exp->len < 2)
                                RECOVER(l, "'let* \"argc < 2 in %S\"", exp);
                        tmp = exp;
                        for(; !isnil(cdr(exp)); exp = cdr(exp)) {
                                if(!iscons(car(exp)) || !cklen(car(exp), 2))
                                   RECOVER(l, "'let* \"expected list of length 2: '%S '%S\"",
                                                   car(exp), tmp);
                                if(first == l->LetRec)
                                        s = env = extend(l, env, CAAR(exp), l->Nil);
                                r = env = extend(l, env, CAAR(exp),
                                        eval(l, depth + 1, CADAR(exp), env));
                                if(first == l->LetRec)
                                        setcdr(car(s), CDAR(r));
                        }
                        return eval(l, depth+1, car(exp), env);
                }
                if(first == l->Begin) { /**@todo Add looping constructs here*/
                        if(isnil(exp)) return l->Nil;
                        for(;;) {
                                if(isnil(cdr(exp))) {
                                      exp = car(exp);
                                      goto tail;
                                }
                                eval(l, depth + 1, car(exp), env);
                                exp = cdr(exp);
                        }
                }

                proc = eval(l, depth + 1, first, env);
                if(isproc(proc) || issubr(proc)) { /*eval their args*/
                        vals = evlis(l, depth + 1, exp, env);
                } else if(isfproc(proc)) { /*f-expr do not eval their args*/
                        vals = cons(l, exp, l->Nil);
                } else if(isin(proc)) { /*special behavior for input ports*/
                        char *s;
                        if(!cklen(exp, 0))
                                RECOVER(l, "\"incorrect application of input port\" %S", exp);
                        if(!(s = io_getline(ioval(proc)))) return l->Error;
                        else return mkstr(l, s);
              /*} else if(isout(proc)) { // todo? */
                } else { 
                        RECOVER(l, "\"not a procedure\" '%S", first);
                }

                if(issubr(proc)) {
                        l->gc_stack_used = gc_stack_save;
                        gc_add(l, proc);
                        gc_add(l, vals);
                        return (*subrval(proc))(l, vals);
                }
                if(isproc(proc) || isfproc(proc)) {
                        if(procargs(proc)->len != vals->len)
                                RECOVER(l, "'proc \"expected %S\" '%S", 
                                                procargs(proc), vals);
                        if(procargs(proc)->len)
                                env = multiple_extend(l, 
                                        l->dynamic ? env : procenv(proc), 
                                        procargs(proc), vals);
                        exp = cons(l, l->Begin, proccode(proc));
                        goto tail;
                }
                RECOVER(l, "\"not a procedure\" '%S", first);
        case INVALID: 
        default:     HALT(l, "%s", "internal inconsistency: unknown type");
        }
        HALT(l, "%s", "internal inconsistency: reached the unreachable");
        return exp; 
}

static cell *evlis(lisp *l, unsigned depth, cell *exps, cell *env) {
        size_t i;
        cell *op, *head;
        if(isnil(exps)) return l->Nil;
        op = car(exps);
        exps = cdr(exps);
        head = op = cons(l, eval(l, depth+1, op, env), l->Nil);
        for(i = 1; !isnil(exps); exps = cdr(exps), op = cdr(op), i++)
                setcdr(op, cons(l, eval(l, depth+1, car(exps), env), l->Nil));
        head->len = i;
        return head;
}

