/**
 *  @file           lisp.c
 *  @brief          The Lisp Interpreter
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v2.0 or later version
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 *  Experimental, small, lisp interpreter.
 *
 *  Meaning of symbols:
 *  i:      input
 *  o:      output
 *  e:      standard error output
 *  x:      expression
 *  args:   a list of *lisp_evaluated* arguments
 *  nx:     a newly created expression
 *
 **/

#include <string.h>             /* strcmp, strlen, strcat */
#include "type.h"               /* includes some std c library headers, as well as types */
#include "io.h"
#include "mem.h"
#include "sexpr.h"
#include "lisp.h"

/** macro helpers **/
#define car(X)      ((X)->data.list[0])
#define cadr(X)     ((X)->data.list[1])
#define caddr(X)    ((X)->data.list[2])
#define cadddr(X)   ((X)->data.list[3])
#define nth(X,Y)    ((X)->data.list[(Y)])
#define tstlen(X,Y) ((Y)==((X)->len))

#define procargs(X) car(X)
#define proccode(X) cadr(X)
#define procenv(X)  caddr(X)

/** global-to-file special objects **/
static expr nil, tee, s_if, s_lambda, s_define, s_begin, s_set, s_quote;

/** functions necessary for the intepreter **/
static expr mkobj(sexpr_e type, io * e);
static expr mksym(char *s, io * e);
static expr mkprimop(expr(*func) (expr args, lisp l), io * e);
static expr mkproc(expr args, expr code, expr env, io * e);
static expr evlis(expr x, expr env, lisp l);
static expr apply(expr proc, expr args, lisp l);        /*add env for dynamic scope? */
static expr dofind(expr env, expr x);   /*add error handling? */
static expr find(expr env, expr x, lisp l);
static expr extend(expr sym, expr val, expr env, io * e);
static char *sdup(const char *s, io * e);
static expr extendprimop(const char *s, expr(*func) (expr args, lisp l), expr env, io * e);
static expr extensions(expr env, expr syms, expr vals, io * e);

/** built in primitives **/
static expr primop_add(expr args, lisp l);
static expr primop_sub(expr args, lisp l);
static expr primop_prod(expr args, lisp l);
static expr primop_div(expr args, lisp l);
static expr primop_mod(expr args, lisp l);
static expr primop_car(expr args, lisp l);
static expr primop_cdr(expr args, lisp l);
static expr primop_cons(expr args, lisp l);
static expr primop_nth(expr args, lisp l);
static expr primop_len(expr args, lisp l);
static expr primop_numeq(expr args, lisp l);
static expr primop_printexpr(expr args, lisp l);
static expr primop_scar(expr args, lisp l);
static expr primop_scdr(expr args, lisp l);
static expr primop_scons(expr args, lisp l);
static expr primop_typeeq(expr args, lisp l);
static expr primop_reverse(expr args, lisp l);
static expr primop_system(expr args, lisp l);

/*
static expr primop_nummore(expr args, lisp l);
static expr primop_nummoreeq(expr args, lisp l);
static expr primop_numless(expr args, lisp l);
static expr primop_numlesseq(expr args, lisp l);
static expr primop_min(expr args, lisp l);
static expr primop_max(expr args, lisp l);
static expr primop_lisp_eval(expr args, lisp l);
static expr primop_read(expr args, lisp l);
static expr primop_getc(expr args, lisp l);
static expr primop_gets(expr args, lisp l);
static expr primop_putc(expr args, lisp l);
static expr primop_find(expr args, lisp l);
*/
/*** interface functions *****************************************************/

/**
 *  @brief          Initialize the lisp interpreter
 *  @return         A fully initialized lisp environment
 **/
lisp lisp_init(void)
{
        lisp l;
        l = mem_calloc(1, sizeof(*l), NULL);
        l->global = mem_calloc(1, sizeof(sexpr_t), NULL);
        l->env = mem_calloc(1, sizeof(sexpr_t), NULL);

        l->i = mem_calloc(1, sizeof(io), NULL);
        l->o = mem_calloc(1, sizeof(io), NULL);
        l->e = mem_calloc(1, sizeof(io), NULL);

        /* set up file I/O and pointers */
        l->i->type = file_in;
        l->i->ptr.file = stdin;
        l->o->type = file_out;
        l->o->ptr.file = stdout;
        l->e->type = file_out;
        l->e->ptr.file = stderr;
        l->global->type = S_LIST;
        l->env->type = S_LIST;

        /* internal symbols */
        nil = mkobj(S_NIL, l->e);
        tee = mkobj(S_TEE, l->e);

        s_if     = mksym(sdup("if", l->e), l->e);
        s_lambda = mksym(sdup("lambda", l->e), l->e);
        s_begin  = mksym(sdup("begin", l->e), l->e);
        s_define = mksym(sdup("define", l->e), l->e);
        s_set    = mksym(sdup("set", l->e), l->e);
        s_quote  = mksym(sdup("quote", l->e), l->e);

        extend(mksym(sdup("nil", l->e), l->e), nil, l->global, l->e);
        extend(mksym(sdup("t", l->e), l->e), tee, l->global, l->e);

        extend(s_if, s_if, l->global, l->e);
        extend(s_lambda, s_lambda, l->global, l->e);
        extend(s_begin, s_begin, l->global, l->e);
        extend(s_define, s_define, l->global, l->e);
        extend(s_set, s_set, l->global, l->e);
        extend(s_quote, s_quote, l->global, l->e);

        /* normal forms, kind of  */
        extendprimop("+", primop_add, l->global, l->e);
        extendprimop("-", primop_sub, l->global, l->e);
        extendprimop("*", primop_prod, l->global, l->e);
        extendprimop("/", primop_div, l->global, l->e);
        extendprimop("mod", primop_mod, l->global, l->e);
        extendprimop("car", primop_car, l->global, l->e);
        extendprimop("cdr", primop_cdr, l->global, l->e);
        extendprimop("cons", primop_cons, l->global, l->e);
        extendprimop("nth", primop_nth, l->global, l->e);
        extendprimop("length", primop_len, l->global, l->e);
        extendprimop("=", primop_numeq, l->global, l->e);
        extendprimop("print", primop_printexpr, l->global, l->e);
        extendprimop("scar", primop_scar, l->global, l->e);
        extendprimop("scdr", primop_scdr, l->global, l->e);
        extendprimop("scons", primop_scons, l->global, l->e);
        extendprimop("eqt", primop_typeeq, l->global, l->e);
        extendprimop("reverse", primop_reverse, l->global, l->e);
        extendprimop("system", primop_system, l->global, l->e);

        return l;
}

/** 
 *  @brief    lisp_repl implements a lisp Read-Evaluate-Print-Loop
 *  @param    l an initialized lisp environment
 *  @return   Always zero at the moment
 *
 *  @todo When Error Expression have been properly implemented any
 *        errors that have not been caught should be returned by lisp_repl
 *        or handled by it to avoid multiple error messages being printed
 *        out.
 */
lisp lisp_repl(lisp l)
{
        expr x;
        while (NULL != (x = sexpr_parse(l->i, l->e))) {
                x = lisp_eval(x, l->env, l);
                sexpr_print(x, l->o, 0, l->e);
                lisp_clean(l);
        }
        return l;
}

/**
 *  @brief          Destroy and clean up a lisp environment
 *  @param          l   initialized lisp environment
 *  @return         void
 **/
void lisp_end(lisp l)
{
        io e = { file_out, {NULL}, 0, 0, '\0', false };
        e.ptr.file = stderr;

        fflush(NULL);

        /*do not call mark before **this** sweep */
        mem_gc_sweep(&e);

        mem_free(l->e, &e);
        mem_free(l->o, &e);
        mem_free(l->i, &e);

        mem_free(l->env, &e);
        mem_free(l->global->data.list, &e);
        mem_free(l->global, &e);
        mem_free(l, &e);

        return;
}

/**
 *  @brief          Read in an s-expression 
 *  @param          i   Read input from...
 *  @param          e   Error stream output
 *  @return         A valid s-expression, which might be an error!
 **/
expr lisp_read(io * i, io * e)
{
        return sexpr_parse(i, e);
}

/**
 *  @brief          Print out an s-expression
 *  @param          x   Expression to print
 *  @param          o   Output stream
 *  @param          e   Error stream output
 *  @return         void
 **/
void lisp_print(expr x, io * o, io * e)
{
        sexpr_print(x, o, 0, e);
        return;
}

/**
 *  @brief          Evaluate an already parsed lisp expression
 *  @param          x   The s-expression to parse
 *  @param          env The environment to lisp_evaluate in
 *  @param          l   The global lisp environment
 *  @return         An lisp_evaluated expression, possibly ready for printing.
 **/
expr lisp_eval(expr x, expr env, lisp l)
{
        unsigned int i;

        if (NULL == x) {
                sexpr_perror(NULL, "passed nul", l->e);
                abort();
        }

        switch (x->type) {
        case S_LIST:
                if (tstlen(x, 0))       /* () */
                        return nil;
                if (S_SYMBOL == car(x)->type) {
                        const expr foundx = lisp_eval(car(x), env, l);
                        if (foundx == s_if) {   /* (if test conseq alt) */
                                if (!tstlen(x, 4)) {
                                        sexpr_perror(x, "if: argc != 4", l->e);
                                        return nil;
                                }
                                if (nil == lisp_eval(cadr(x), env, l)) {
                                        return lisp_eval(cadddr(x), env, l);
                                } else {
                                        return lisp_eval(caddr(x), env, l);
                                }
                        } else if (foundx == s_begin) { /* (begin exp ... ) */
                                if (tstlen(x, 1)) {
                                        return nil;
                                }
                                for (i = 1; i < x->len - 1; i++) {
                                        lisp_eval(nth(x, i), env, l);
                                }
                                return lisp_eval(nth(x, i), env, l);
                        } else if (foundx == s_quote) { /* (quote exp) */
                                if (!tstlen(x, 2)) {
                                        sexpr_perror(x, "quote: argc != 1", l->e);
                                        return nil;
                                }
                                return cadr(x);
                        } else if (foundx == s_set) {
                                expr nx;
                                if (!tstlen(x, 3)) {
                                        sexpr_perror(x, "set: argc != 2", l->e);
                                        return nil;
                                }
                                nx = find(env, cadr(x), l);
                                if (nil == nx) {
                                        sexpr_perror(cadr(x), "unbound symbol", l->e);
                                        return nil;
                                }
                                nx->data.list[1] = lisp_eval(caddr(x), env, l);
                                return cadr(nx);
                        } else if (foundx == s_define) {
                                {
                                        expr nx;
                                        /*@todo if already defined, or is an internal symbol, report error */
                                        if (!tstlen(x, 3)) {
                                                sexpr_perror(x, "define: argc != 2", l->e);
                                                return nil;
                                        }
                                        nx = extend(cadr(x), lisp_eval(caddr(x), env, l), l->global, l->e);
                                        return nx;
                                }
                        } else if (foundx == s_lambda) {
                                if (!tstlen(x, 3)) {
                                        sexpr_perror(x, "lambda: argc != 2", l->e);
                                        return nil;
                                }
                                return mkproc(cadr(x), caddr(x), env, l->e);
                        } else {
                                return apply(foundx, evlis(x, env, l), l);
                        }
                } else {
                        sexpr_perror(car(x), "cannot apply", l->e);
                        return nil;
                }
                break;
        case S_SYMBOL:         /*if symbol found, return it, else error; unbound symbol */
                {
                        expr nx = find(env, x, l);
                        if (nil == nx) {
                                sexpr_perror(x, "unbound symbol", l->e);
                                return nil;
                        }
                        return cadr(nx);
                }
        case S_FILE:           /* to implement */
                sexpr_perror(NULL, "file type unimplemented", l->e);
                return nil;
        case S_NIL:
        case S_TEE:
        case S_STRING:
        case S_PROC:
        case S_INTEGER:
        case S_PRIMITIVE:
                return x;
        default:
                sexpr_perror(NULL, "fatal: unknown type", l->e);
                abort();
        }

        sexpr_perror(NULL, "should never get here", l->e);
        return x;
}

/**
 *  @brief          Garbage collection
 *  @param          l   Lisp environment to mark and collect
 *  @return         void
 **/
void lisp_clean(lisp l)
{
        mem_gc_mark(l->global, l->e);
        mem_gc_sweep(l->e);
}

/*** internal functions ******************************************************/

/**find a symbol in an environment and the global list**/
static expr find(expr env, expr x, lisp l)
{
        expr nx = dofind(env, x);
        if (nil == nx) {
                nx = dofind(l->global, x);
                if (nil == nx) {
                        return nil;
                }
        }
        return nx;
}

/** find a symbol in a special type of list **/
static expr dofind(expr env, expr x)
{
  /** @todo make this function more robust **/
        unsigned int i;
        char *s;
        if (S_LIST != env->type || S_SYMBOL != x->type) {
                return nil;
        }
        s = x->data.symbol;
        if (1 > env->len) {
                return nil;
        }
        for (i = env->len - 1; i != 0; i--) {
                                       /** reverse search order **/
                if (!strcmp(car(nth(env, i))->data.symbol, s)) {
                        return nth(env, i);
                }
        }
        if (!strcmp(car(nth(env, 0))->data.symbol, s)) {
                return nth(env, 0);
        }
        return nil;
}

/** extend a lisp environment **/
static expr extend(expr sym, expr val, expr env, io * e)
{
        expr nx = mkobj(S_LIST, e);
        append(nx, sym, e);
        append(nx, val, e);
        append(env, nx, e);
        return val;
}

/** duplicate a string **/
static char *sdup(const char *s, io * e)
{
        char *ns;
        if (NULL == s) {
                sexpr_perror(NULL, "passed NULL", e);
        }
        ns = _malloc(sizeof(char) * (strlen(s) + 1), e);
        strcpy(ns, s);
        return ns;
}

/** extend the lisp environment with a primitive operator **/
static expr extendprimop(const char *s, expr(*func) (expr args, lisp l), expr env, io * e)
{
        return extend(mksym(sdup(s, e), e), mkprimop(func, e), env, e);
}

/** make new object **/
static expr mkobj(sexpr_e type, io * e)
{
        expr nx;
        nx = mem_gc_calloc(e);
        nx->len = 0;
        nx->type = type;
        return nx;
}

/** make a new symbol **/
static expr mksym(char *s, io * e)
{
        expr nx;
        nx = mkobj(S_SYMBOL, e);
        nx->len = strlen(s);
        nx->data.symbol = s;
        return nx;
}

/** make a new primitive **/
static expr mkprimop(expr(*func) (expr args, lisp l), io * e)
{
        expr nx;
        nx = mkobj(S_PRIMITIVE, e);
        nx->data.func = func;
        return nx;
}

/** make a new process **/
static expr mkproc(expr args, expr code, expr env, io * e)
{
  /** 
   *  @todo check all args are symbols, clean this up! 
   **/
        expr nx, nenv;
        nx = mkobj(S_PROC, e);
        append(nx, args, e);
        append(nx, code, e);
  /** @todo turn into mklist **/
        nenv = mkobj(S_LIST, e);
        nenv->data.list = _malloc(env->len * sizeof(expr), e);
        memcpy(nenv->data.list, env->data.list, (env->len) * sizeof(expr));
        nenv->len = env->len;
  /****************************/
        append(nx, nenv, e);
        return nx;
}

/** lisp_evaluate a list **/
static expr evlis(expr x, expr env, lisp l)
{
        unsigned int i;
        expr nx;
        nx = mkobj(S_LIST, l->e);
        for (i = 1 /*skip 0! */ ; i < x->len; i++) {
                append(nx, lisp_eval(nth(x, i), env, l), l->e);
        }
        return nx;
}

/** extend a enviroment with symbol/value pairs **/
static expr extensions(expr env, expr syms, expr vals, io * e)
{
        unsigned int i;
        if (0 == vals->len)
                return nil;

        for (i = 0; i < syms->len; i++) {
                extend(nth(syms, i), nth(vals, i), env, e);
        }
        return env;
}

/** apply a procedure over arguments given an environment **/
static expr apply(expr proc, expr args, lisp l)
{
        expr nenv;
        if (S_PRIMITIVE == proc->type) {
                return (proc->data.func) (args, l);
        }
        if (S_PROC == proc->type) {
                if (args->len != procargs(proc)->len) {
                        sexpr_perror(args, "expected number of args incorrect", l->e);
                        return nil;
                }
                nenv = extensions(procenv(proc), procargs(proc), args, l->e);
                /*sexpr_print(nenv,l->o,0,l->e);//might be useful to keep in! */
                return lisp_eval(proccode(proc), nenv, l);
        }

        sexpr_perror(proc, "apply failed", l->e);
        return nil;
}

/*** primitive operations ****************************************************/

/**macro helpers for primops**/
#define intchk(EXP,ERR)\
  if(S_INTEGER!=((EXP)->type)){\
    sexpr_perror((EXP),"arg != integer",(ERR));\
    return nil;\
  }

/**add a list of numbers**/
static expr primop_add(expr args, lisp l)
{
        unsigned int i;
        expr nx = mkobj(S_INTEGER, l->e);
        if (0 == args->len)
                return nil;
        for (i = 0; i < args->len; i++) {
                intchk(nth(args, i), l->e);
                nx->data.integer += (nth(args, i)->data.integer);
        }
        return nx;
}

/**subtract a list of numbers from the 1 st arg**/
static expr primop_sub(expr args, lisp l)
{
        unsigned int i;
        expr nx = mkobj(S_INTEGER, l->e);
        if (0 == args->len)
                return nil;
        nx = nth(args, 0);
        intchk(nx, l->e);
        for (i = 1; i < args->len; i++) {
                intchk(nth(args, i), l->e);
                nx->data.integer -= (nth(args, i)->data.integer);
        }
        return nx;
}

/**multiply a list of numbers together**/
static expr primop_prod(expr args, lisp l)
{
        unsigned int i;
        expr nx = mkobj(S_INTEGER, l->e);
        if (0 == args->len)
                return nil;
        nx = nth(args, 0);
        intchk(nx, l->e);
        for (i = 1; i < args->len; i++) {
                intchk(nth(args, i), l->e);
                nx->data.integer *= (nth(args, i)->data.integer);
        }
        return nx;
}

/**divide the first argument by a list of numbers**/
static expr primop_div(expr args, lisp l)
{
        unsigned int i, tmp;
        expr nx = mkobj(S_INTEGER, l->e);
        if (0 == args->len)
                return nil;
        nx = nth(args, 0);
        intchk(nx, l->e);
        for (i = 1; i < args->len; i++) {
                intchk(nth(args, i), l->e);
                tmp = nth(args, i)->data.integer;
                if (tmp) {
                        nx->data.integer /= tmp;
                } else {
                        sexpr_perror(args, "div: 0/", l->e);
                        return nil;
                }
        }
        return nx;
}

/**arg_1 modulo arg_2**/
static expr primop_mod(expr args, lisp l)
{
        unsigned int tmp;
        expr nx = mkobj(S_INTEGER, l->e);
        if (2 != args->len) {
                sexpr_perror(args, "mod: argc != 2", l->e);
                return nil;
        }
        intchk(car(args), l->e);
        intchk(cadr(args), l->e);

        tmp = cadr(args)->data.integer;
        if (0 == tmp) {
                sexpr_perror(args, "mod: 0/", l->e);
                return nil;
        }
        nx->data.integer = car(args)->data.integer % tmp;

        return nx;
}

/**car**/
static expr primop_car(expr args, lisp l)
{
        expr a1;
        if (1 != args->len) {
                sexpr_perror(args, "car: argc != 1", l->e);
                return nil;
        }

        a1 = car(args);
        if (S_LIST != a1->type) {
                sexpr_perror(args, "args != list", l->e);
                return nil;
        }
        return car(a1);
}

/**cdr**/
static expr primop_cdr(expr args, lisp l)
{
        expr nx, carg;
        if (0 == args->len) {
                return nil;
        }
        nx = mkobj(S_LIST, l->e);
        carg = car(args);
        if ((S_LIST != carg->type) || (1 >= carg->len)) {
                return nil;
        }

        nx->data.list = _malloc((carg->len - 1) * sizeof(expr), l->e);
        memcpy(nx->data.list, carg->data.list + 1, (carg->len - 1) * sizeof(expr));
        nx->len = carg->len - 1;
        return nx;
}

/**cons**/
static expr primop_cons(expr args, lisp l)
{
        expr nx = mkobj(S_LIST, l->e), prepend, list;
        if (2 != args->len) {
                sexpr_perror(args, "cons: argc != 2", l->e);
                return nil;
        }
        prepend = car(args);
        list = cadr(args);
        if (S_NIL == list->type) {
                append(nx, prepend, l->e);
        } else if (S_LIST == list->type) {
    /** @todo turn into mklist **/
                nx->data.list = _malloc((list->len + 1) * sizeof(expr), l->e);
                car(nx) = prepend;
                memcpy(nx->data.list + 1, list->data.list, (list->len) * sizeof(expr));
                nx->len = list->len + 1;
    /****************************/
        } else {
                append(nx, prepend, l->e);
                append(nx, list, l->e);
        }
        return nx;
}

/**nth element in a list or string**/
static expr primop_nth(expr args, lisp l)
{
        cell_t i;
        expr a1, a2;
        if (2 != args->len) {
                sexpr_perror(args, "nth: argc != 2", l->e);
                return nil;
        }
        a1 = car(args);
        a2 = cadr(args);
        if (S_INTEGER != a1->type) {
                sexpr_perror(args, "nth: arg 1 != integer", l->e);
                return nil;
        }
        if ((S_LIST != a2->type) && (S_STRING != a2->type)) {
                sexpr_perror(args, "nth: arg 2 != list || string", l->e);
                return nil;
        }

        i = car(args)->data.integer;
        if (0 > i) {
                i = a2->len + i;
        }

        if ((unsigned)i > a2->len) {
                return nil;
        }

        if (S_LIST == a2->type) {
                return nth(a2, i);
        } else {                /*must be string */
                {
                        expr nx = mkobj(S_STRING, l->e);
                        nx->data.string = mem_calloc(sizeof(char), 2, l->e);
                        nx->data.string[0] = a2->data.string[i];
                        nx->len = 1;
                        return nx;
                }
        }
        return nil;
}

/**length of a list or string**/
static expr primop_len(expr args, lisp l)
{
        expr a1, nx = mkobj(S_INTEGER, l->e);
        if (1 != args->len) {
                sexpr_perror(args, "len: argc != 1", l->e);
                return nil;
        }
        a1 = car(args);
        if ((S_LIST != a1->type) && (S_STRING != a1->type)) {
                sexpr_perror(args, "len: arg 2 != list || string", l->e);
                return nil;
        }
        nx->data.integer = a1->len;
        return nx;
}

/**test equality of the 1st arg against a list of numbers**/
static expr primop_numeq(expr args, lisp l)
{
        unsigned int i;
        expr nx;
        if (0 == args->len)
                return nil;
        nx = nth(args, 0);
        intchk(nx, l->e);
        for (i = 1; i < args->len; i++) {
                intchk(nth(args, i), l->e);
                if (nx->data.integer != (nth(args, i)->data.integer)) {
                        return nil;
                }
                nx->data.integer = (nth(args, i)->data.integer);
        }
        return tee;
}

/**print**/
static expr primop_printexpr(expr args, lisp l)
{
        /* @todo if arg = 1, treat as Output redirection, else normal out */
        sexpr_print(args, l->o, 0, l->e);
        return args;
}

/**car for strings**/
static expr primop_scar(expr args, lisp l)
{
        expr nx, a1;
        if (1 != args->len) {
                sexpr_perror(args, "car: argc != 1", l->e);
                return nil;
        }
        a1 = car(args);
        if (S_STRING != a1->type) {
                sexpr_perror(args, "args != string", l->e);
                return nil;
        }
        nx = mkobj(S_STRING, l->e);
        nx->data.string = mem_calloc(sizeof(char), 2, l->e);
        nx->data.string[0] = a1->data.string[0];
        nx->len = 1;
        return nx;
}

/**cdr for strings**/
static expr primop_scdr(expr args, lisp l)
{
        expr nx, carg;
        if (0 == args->len) {
                return nil;
        }
        nx = mkobj(S_STRING, l->e);
        carg = car(args);
        if ((S_STRING != carg->type) || (1 >= carg->len)) {
                return nil;
        }

        nx->data.string = mem_calloc(sizeof(char), carg->len, l->e);       /*not len + 1 */
        strcpy(nx->data.string, carg->data.string + 1);
        nx->len = carg->len - 1;
        return nx;
}

/**cons for strings**/
static expr primop_scons(expr args, lisp l)
{
        expr nx = mkobj(S_LIST, l->e), prepend, list;
        if (2 != args->len) {
                sexpr_perror(args, "cons: argc != 2", l->e);
                return nil;
        }
        prepend = car(args);
        list = cadr(args);
        if ((S_STRING == list->type) && (S_STRING == prepend->type)) {
                nx->type = S_STRING;
                nx->data.string = mem_calloc(sizeof(char), (list->len) + (prepend->len) + 1, l->e);
                nx->len = list->len + prepend->len;
                strcpy(nx->data.string, prepend->data.string);
                strcat(nx->data.string, list->data.string);
        } else {
                sexpr_perror(args, "cons: arg != string", l->e);
                return nil;
        }
        return nx;
}

/**type equality**/
static expr primop_typeeq(expr args, lisp l)
{
        unsigned int i;
        expr nx;
        if (args == NULL) {     /*here to supress warning */
                sexpr_perror(args, "eqt: passed NULL", l->e);
                return nil;
        }

        if (0 == args->len)
                return nil;
        nx = nth(args, 0);
        for (i = 1; i < args->len; i++) {
                if (nx->type != (nth(args, i)->type)) {
                        return nil;
                }
        }
        return tee;
}

/**reverse a list or a string**/
static expr primop_reverse(expr args, lisp l)
{
        unsigned int i;
        expr nx, carg;
        sexpr_e type;
        if (1 != args->len) {
                sexpr_perror(args, "reverse: argc != 1", l->e);
                return nil;
        }
        carg = car(args);
        type = carg->type;
        if ((S_LIST != type) && (S_STRING != type)) {
                sexpr_perror(args, "reverse: not a reversible type", l->e);
                return nil;
        }
        nx = mkobj(type, l->e);

        if (S_LIST == type) {
                nx->data.list = _malloc(carg->len * sizeof(expr), l->e);
                for (i = 0; i < carg->len; i++) {
                        nth(nx, carg->len - i - 1) = nth(carg, i);
                }
                nx->len = carg->len;
        } else {                /*is a string */
                nx->data.string = mem_calloc(sizeof(char), carg->len * sizeof(expr) + 1, l->e);
                for (i = 0; i < carg->len; i++) {
                        nx->data.string[carg->len - i - 1] = carg->data.string[i];
                }
                nx->len = carg->len;
        }
        return nx;
}

static expr primop_system(expr args, lisp l){
        int i;
        expr nx, carg;

        if (1 != args->len) {
                sexpr_perror(args, "system: argc != 1", l->e);
                return nil;
        }
        carg = car(args);
        if(S_STRING != carg->type){
                sexpr_perror(args, "system: arg != string", l->e);
                return nil;
        }
        i = system(carg->data.string);
        if(0 > i)
                return nil;

        nx = mkobj(S_INTEGER, l->e);
        nx->data.integer = i;

        return nx;
}

#undef intchk

/*****************************************************************************/
