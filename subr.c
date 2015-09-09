/** @file       subr.c
 *  @brief      The liblisp interpreter built in subroutines
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *
 *  This file is the largest one in the project, this normally would be a
 *  problem, however all of the functions (apart from lisp_init and related
 *  functions) follow the same pattern, apart from a very few exceptions they
 *  do not call each other. All of the functions named subr_X (X being the rest
 *  of the function name) abide by the same interface as they are built in
 *  subroutines for the lisp interpreter.
 *
 *  This file is uses X-Macros to populated tables of subroutines and variables
 *  along with their associated names.
 *
 *  Assertions should be added throughout these routines to make sure
 *  operations do not go out of bounds.
 *
 *  While each function has a very short description about what it does the
 *  manual (which should be called something like "liblisp.md") describes
 *  what each subroutine primitive does within the context of the interpreter.
 **/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <ctype.h>
#include <locale.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*X-Macro of primitive functions and their names; basic built in subr*/
#define SUBROUTINE_XLIST\
        X(subr_band,    "&")              X(subr_bor,       "|")\
        X(subr_bxor,    "^")              X(subr_binv,      "~")\
        X(subr_sum,     "+")              X(subr_sub,       "-")\
        X(subr_prod,    "*")              X(subr_mod,       "%")\
        X(subr_div,     "/")              X(subr_eq,        "=")\
        X(subr_eq,      "eq")             X(subr_greater,   ">")\
        X(subr_less,    "<")              X(subr_cons,      "cons")\
        X(subr_car,     "car")            X(subr_cdr,       "cdr")\
        X(subr_list,    "list")           X(subr_match,     "match")\
        X(subr_scons,   "scons")          X(subr_scar,      "scar")\
        X(subr_scdr,    "scdr")           X(subr_eval,      "eval")\
        X(subr_gc,      "gc")             X(subr_trace_level, "trace-level!")\
        X(subr_length,  "length")         X(subr_typeof,    "type-of")\
        X(subr_inp,     "input?")         X(subr_outp,      "output?")\
        X(subr_eofp,    "eof?")           X(subr_flush,     "flush")\
        X(subr_tell,    "tell")           X(subr_seek,      "seek")\
        X(subr_close,   "close")          X(subr_open,      "open")\
        X(subr_getchar, "get-char")       X(subr_getdelim,  "get-delim")\
        X(subr_read,    "read")           X(subr_puts,      "put")\
        X(subr_putchar, "put-char")       X(subr_print,     "print")\
        X(subr_ferror,  "ferror")         X(subr_system,    "system")\
        X(subr_remove,  "remove")         X(subr_rename,    "rename")\
        X(subr_hlookup, "hash-lookup")    X(subr_hinsert,   "hash-insert")\
        X(subr_coerce,  "coerce")         X(subr_time,      "time")\
        X(subr_getenv,  "getenv")         X(subr_rand,      "random")\
        X(subr_seed,    "seed")           X(subr_date,      "date")\
        X(subr_assoc,   "assoc")          X(subr_setlocale, "locale!")\
        X(subr_trace_cell, "trace")       X(subr_binlog,    "binary-logarithm")\
        X(subr_timed_eval, "timed-eval")  X(subr_reverse,   "reverse")\
        X(subr_join,    "join")           X(subr_regexspan, "regex-span")\
        X(subr_raise,   "raise")          X(subr_split,     "split")\
        X(subr_hcreate, "hash-create")    X(subr_format,    "format")\
        X(subr_substring, "substring")    X(subr_tr,        "tr")

#define X(SUBR, NAME) static cell* SUBR (lisp *l, cell *args);
SUBROUTINE_XLIST /*function prototypes for all of the built-in subroutines*/
#undef X

#define X(SUBR, NAME) { SUBR, NAME },
static struct all_subroutines { subr p; char *name; } primitives[] = {
        SUBROUTINE_XLIST /*all of the subr functions*/
        {NULL, NULL} /*must be terminated with NULLs*/
};
#undef X

/**< X-Macros of all built in integers*/
#define INTEGER_XLIST\
        X("*seek-cur*",     SEEK_CUR)     X("*seek-set*",    SEEK_SET)\
        X("*seek-end*",     SEEK_END)     X("*random-max*",  INTPTR_MAX)\
        X("*integer-max*",  INTPTR_MAX)   X("*integer-min*", INTPTR_MIN)\
        X("*integer*",      INTEGER)      X("*symbol*",      SYMBOL)\
        X("*cons*",         CONS)         X("*string*",      STRING)\
        X("*hash*",         HASH)         X("*io*",          IO)\
        X("*float*",        FLOAT)        X("*procedure*",   PROC)\
        X("*primitive*",    SUBR)         X("*f-procedure*", FPROC)\
        X("*file-in*",      FIN)          X("*file-out*",    FOUT)\
        X("*string-in*",    SIN)          X("*string-out*",  SOUT)\
        X("*lc-all*",       LC_ALL)       X("*lc-collate*",  LC_COLLATE)\
        X("*lc-ctype*",     LC_CTYPE)     X("*lc-monetary*", LC_MONETARY)\
        X("*lc-numeric*",   LC_NUMERIC)   X("*lc-time*",     LC_TIME)\
        X("*user-defined*", USERDEF)      X("*trace-off*",   TRACE_OFF)\
        X("*trace-marked*", TRACE_MARKED) X("*trace-all*",   TRACE_ALL)\
        X("*gc-on*",        GC_ON)        X("*gc-postpone*", GC_POSTPONE)\
        X("*gc-off*",       GC_OFF)       X("*eof*",         EOF)\
        X("*sig-abrt*",     SIGABRT)      X("*sig-fpe*",     SIGFPE)\
        X("*sig-ill*",      SIGILL)       X("*sig-int*",     SIGINT)\
        X("*sig-segv*",     SIGSEGV)      X("*sig-term*",    SIGTERM)

#define X(NAME, VAL) { NAME, VAL }, 
static struct integer_list { char *name; intptr_t val; } integers[] = {
        INTEGER_XLIST
        {NULL, 0}
};
#undef X

#define X(CNAME, LNAME) static cell _ ## CNAME = { SYMBOL, 0, 0, 1, 0, 0, 0, .p[0].v = LNAME};
CELL_XLIST /*structs for special cells*/
#undef X

#define X(CNAME, NOT_USED) static cell* CNAME = & _ ## CNAME;
CELL_XLIST /*pointers to structs for special cells*/
#undef X

#define X(CNAME, NOT_USED) { & _ ## CNAME },
/**@brief a list of all the special symbols**/
static struct special_cell_list { cell *internal; } special_cells[] = {
        CELL_XLIST 
        { NULL }
};
#undef X

#define SUBR_ISX(NAME)\
static cell *subr_ ## NAME (lisp *l, cell *args) {\
        char *s, c;\
        if(cklen(args, 1) && is_int(car(args)))\
                return NAME (intval(car(args))) ? gsym_tee() : gsym_nil();\
        if(!cklen(args, 1) || !is_asciiz(car(args)))\
                RECOVER(l, "\"expected (string)\" %S", args);\
        s = strval(car(args));\
        if(!s[0]) return gsym_nil();\
        while((c = *s++)) \
                if(! NAME (c))\
                        return gsym_nil();\
        return gsym_tee();\
}

#define ISX_LIST\
        X(isalnum) X(isalpha) X(iscntrl)\
        X(isdigit) X(isgraph) X(islower)\
        X(isprint) X(ispunct) X(isspace)\
        X(isupper) X(isxdigit)

/*defines functions to get a lisp "cell" for the built in special symbols*/
#define X(FNAME, IGNORE) cell *gsym_ ## FNAME (void) { return FNAME ; }
CELL_XLIST
#undef X

#define X(FUNC) SUBR_ISX(FUNC)
ISX_LIST /*defines lisp subroutines for checking whether a string only contains a character class*/
#undef X

lisp *lisp_init(void) {
        lisp *l;
        unsigned i;
        if(!(l = calloc(1, sizeof(*l))))       goto fail;
        if(!(l->ifp = io_fin(stdin)))          goto fail;
        if(!(l->ofp = io_fout(stdout)))        goto fail;
        if(!(l->efp = io_fout(stderr)))        goto fail;
        if(!(l->buf = calloc(DEFAULT_LEN, 1))) goto fail;
        l->buf_allocated = DEFAULT_LEN;
        if(!(l->gc_stack = calloc(DEFAULT_LEN, sizeof(*l->gc_stack)))) 
                goto fail;
        l->gc_stack_allocated = DEFAULT_LEN;
        l->max_depth          = LARGE_DEFAULT_LEN; /**< max recursion depth*/

#define X(CNAME, LNAME) l-> CNAME = CNAME;
CELL_XLIST
#undef X

        l->random_state[0] = 0xCAFE; /*Are these good seeds?*/
        l->random_state[1] = 0xBABE; 
        for(i = 0; i < LARGE_DEFAULT_LEN; i++) /*discard first N numbers*/
                (void)xorshift128plus(l->random_state);

        /* The lisp init function is now ready to add built in subroutines
         * and other variables, the order in which is does this matters. */

        if(!(l->all_symbols = mkhash(l, hash_create(LARGE_DEFAULT_LEN)))) goto fail;
        if(!(l->top_env = cons(l, cons(l, gsym_nil(), gsym_nil()), gsym_nil()))) goto fail;

        for(i = 0; special_cells[i].internal; i++) /*add special cells*/
                if(!lisp_intern(l, special_cells[i].internal))
                        goto fail;

        if(!extend_top(l, gsym_tee(), gsym_tee())) goto fail;

        if(!lisp_add_cell(l, "pi", mkfloat(l, 3.14159265358979323846))) goto fail;
        if(!lisp_add_cell(l, "e",  mkfloat(l, 2.71828182845904523536))) goto fail;

        if(!lisp_add_cell(l, "*stdin*",  mkio(l, io_fin(stdin))))   goto fail;
        if(!lisp_add_cell(l, "*stdout*", mkio(l, io_fout(stdout)))) goto fail;
        if(!lisp_add_cell(l, "*stderr*", mkio(l, io_fout(stderr)))) goto fail;

        for(i = 0; integers[i].name; i++) /*add all integers*/
                if(!lisp_add_cell(l, integers[i].name, mkint(l, integers[i].val)))
                        goto fail;
        for(i = 0; primitives[i].p; i++) /*add all primitives*/
                if(!lisp_add_subr(l, primitives[i].name, primitives[i].p))
                        goto fail;

#define X(FUNC) if(!lisp_add_subr(l, # FUNC "?", subr_ ## FUNC)) goto fail;
ISX_LIST
#undef X

        return l;
fail:   lisp_destroy(l);
        return NULL;
}

static cell *subr_band(lisp *l, cell *args) { /**< bitwise and of two integers*/
        if(!cklen(args, 2) || !is_int(car(args)) || !is_int(CADR(args)))
                RECOVER(l, "\"expected (int int)\" '%S", args);
        return mkint(l, intval(car(args)) & intval(CADR(args)));
}

static cell *subr_bor(lisp *l, cell *args) { /**< bitwise or of two integers*/
        if(!cklen(args, 2) || !is_int(car(args)) || !is_int(CADR(args)))
                RECOVER(l, "\"expected (int int)\" '%S", args);
        return mkint(l, intval(car(args)) | intval(CADR(args)));
}

static cell *subr_bxor(lisp *l, cell *args) { /**< bitwise x-or of two integers*/
        if(!cklen(args, 2) || !is_int(car(args)) || !is_int(CADR(args)))
                RECOVER(l, "\"expected (int int)\" '%S", args);
        return mkint(l, intval(car(args)) ^ intval(CADR(args)));
}

static cell *subr_binv(lisp *l, cell *args) { /**< bitwise inversion of integer*/
        if(!cklen(args, 1) || !is_int(car(args)))
                RECOVER(l, "\"expected (int)\" '%S", args);
        return mkint(l, ~intval(car(args)));
}

static cell *subr_binlog(lisp *l, cell *args) { /**< binary logarithm of integer*/
        if(!cklen(args, 1) || !is_int(car(args)))
                RECOVER(l, "\"expected (int)\" '%S", args);
        return mkint(l, binlog(intval(car(args))));
}

static cell *subr_sum(lisp *l, cell *args) { /**< add two numbers*/
        if(!cklen(args, 2)) 
                RECOVER(l, "\"argument count not equal 2\" '%S", args);
        cell *x = car(args), *y = CADR(args);
        if(is_int(x) && is_arith(y)) {
                if(is_floatval(y)) return mkint(l, intval(x) + floatval(y));
                else           return mkint(l, intval(x) + intval(y));
        } else if(is_floatval(x) && is_arith(y)) {
                if(is_floatval(y)) return mkfloat(l, floatval(x) + floatval(y));
                else           return mkfloat(l, floatval(x) + (lfloat) intval(y));
        }
        RECOVER(l, "\"type check problem\" %S", args);
        return gsym_error();
}

static cell *subr_sub(lisp *l, cell *args) { /**< take one number away from another*/
        if(!cklen(args, 2)) 
                RECOVER(l, "\"argument count not equal 2\" '%S", args);
        cell *x = car(args), *y = CADR(args);
        if(is_int(x) && is_arith(y)) {
                if(is_floatval(y)) return mkint(l, intval(x) - floatval(y));
                else           return mkint(l, intval(x) - intval(y));
        } else if(is_floatval(x) && is_arith(y)) {
                if(is_floatval(y)) return mkfloat(l, floatval(x) - floatval(y));
                else           return mkfloat(l, floatval(x) - (lfloat) intval(y));
        }
        RECOVER(l, "\"type check failed\" '%S", args);
        return gsym_error(); 
}

static cell *subr_prod(lisp *l, cell *args) { /*multiply two numbers*/
        if(!cklen(args, 2)) 
                RECOVER(l, "\"argument count not equal 2\" '%S", args);
        cell *x = car(args), *y = CADR(args);
        if(is_int(x) && is_arith(y)) {
                if(is_floatval(y)) return mkint(l, intval(x) * floatval(y));
                else           return mkint(l, intval(x) * intval(y));
        } else if(is_floatval(x) && is_arith(y)) {
                if(is_floatval(y)) return mkfloat(l, floatval(x) * floatval(y));
                else           return mkfloat(l, floatval(x) * (lfloat) intval(y));
        }
        RECOVER(l, "\"type check failed\" '%S", args);
        return gsym_error();
}

static cell *subr_mod(lisp *l, cell *args) { /*modulo operation*/
        intptr_t dividend, divisor;
        if(!cklen(args, 2) || !is_int(car(args)) || !is_int(CADR(args)))
                RECOVER(l, "\"argument count not equal 2\" '%S", args);
        dividend = intval(car(args));
        divisor  = intval(CADR(args));
        if(!divisor || (dividend == INTPTR_MIN && divisor == -1)) 
                RECOVER(l, "\"invalid divisor values\" '%S", args);
        return mkint(l, dividend % divisor);
}

static cell *subr_div(lisp *l, cell *args) { /**< divis_ion*/
        if(!cklen(args, 2))
                RECOVER(l, "\"argument count not equal 2\" '%S", args);
        if(is_int(car(args)) && is_arith(CADR(args))) {
                intptr_t dividend, divisor;
                dividend = intval(car(args));
                divisor = is_floatval(CADR(args)) ? 
                        floatval(CADR(args)) : 
                        intval(CADR(args));
                if(!divisor || (dividend == INTPTR_MIN && divisor == -1))
                        RECOVER(l, "\"invalid divisor values\" '%S", args);
                return mkint(l, dividend / divisor);
        } else if(is_floatval(car(args)) && is_arith(CADR(args))) {
                lfloat dividend, divisor;
                dividend = floatval(car(args));
                divisor = is_floatval(CADR(args)) ? 
                        floatval(CADR(args)) : 
                        intval(CADR(args));
                if(divisor == 0.)
                        RECOVER(l, "\"divis_ion by zero in %S\"", args);
                return mkfloat(l, dividend / divisor);
        }
        RECOVER(l, "\"type check failed\" '%S", args);
        return gsym_error();
}

static cell *subr_greater(lisp *l, cell *args) { /**< number or string comparison; greater */
        cell *x, *y;
        if(!cklen(args, 2))
                RECOVER(l, "\"expected (number number) or (string string)\" '%S", args);
        x = car(args);
        y = CADR(args);
        if(is_arith(x) && is_arith(y))
                return  (is_floatval(x) ? floatval(x) : intval(x)) > 
                        (is_floatval(y) ? floatval(y) : intval(y)) ? gsym_tee() : gsym_nil();
        else if(is_asciiz(x) && is_asciiz(x))
                return (strcmp(strval(x), strval(y)) > 0) ? gsym_tee() : gsym_nil();
        RECOVER(l, "\"expected (number number) or (string string)\" '%S", args);
        return gsym_error();
}

static cell *subr_less(lisp *l, cell *args) { /**< number or string comparison; less */
        cell *x, *y;
        if(!cklen(args, 2))
                RECOVER(l, "\"expected (number number) or (string string)\" '%S", args);
        x = car(args);
        y = CADR(args);
        if(is_arith(x) && is_arith(y))
                return  (is_floatval(x) ? floatval(x) : intval(x)) < 
                        (is_floatval(y) ? floatval(y) : intval(y)) ? gsym_tee() : gsym_nil();
        else if(is_asciiz(x) && is_asciiz(x))
                return (strcmp(strval(x), strval(y)) < 0) ? gsym_tee() : gsym_nil();
        RECOVER(l, "\"expected (number number) or (string string)\" '%S", args);
        return gsym_error();
}

static cell *subr_eq(lisp *l, cell *args) { /**< equality of two atoms*/
        cell *x, *y;
        if(!cklen(args, 2))
                RECOVER(l, "'arg-count \"argc != 2 in %S\"", args);
        x = car(args);
        y = CADR(args);
        if(is_userdef(x) && l->ufuncs[x->userdef].equal)
                return (l->ufuncs[x->userdef].equal)(x, y) ? gsym_tee() : gsym_nil();
        if(intval(x) == intval(y))
                return gsym_tee();
        if(is_str(x) && is_str(y)) {
                if(!strcmp(strval(x), strval(y))) return gsym_tee();
                else return gsym_nil();
        }
        return gsym_nil();
}

static cell *subr_cons(lisp *l, cell *args) { /**< allocate a new cons cell*/
        if(!cklen(args, 2))
                RECOVER(l, "\"expected (expr expr)\" '%S", args);
        return cons(l, car(args), CADR(args)); 
}

static cell *subr_car(lisp *l, cell *args) { /**< get first element of cons cell*/
        if(!cklen(args, 1) || !is_cons(car(args)))
                RECOVER(l, "\"expect (list)\" '%S", args);
        return CAAR(args); 
}

static cell *subr_cdr(lisp *l, cell *args) { /**< get second element of cons cell*/
        if(!cklen(args, 1) || !is_cons(car(args)))
                RECOVER(l, "\"argument count not equal 1 or not a list\" '%S", args);
        return CDAR(args); 
}

static cell *subr_list(lisp *l, cell *args) { /**< create list out of arguments*/
        size_t i;
        cell *op, *head;
        if(cklen(args, 0))
                RECOVER(l, "\"argument count must be more than 0\" '%S", args);
        op = car(args);
        args = cdr(args);
        head = op = cons(l, op, gsym_nil());
        for(i = 1; !is_nil(args); args = cdr(args), op = cdr(op), i++)
                setcdr(op, cons(l, car(args), gsym_nil()));
        head->len = i;
        return head;
}

static cell *subr_match(lisp *l, cell *args) { /**< very simple match operation on string given pattern*/
        if(!cklen(args, 2) 
        || !is_asciiz(car(args)) || !is_asciiz(CADR(args))) 
                RECOVER(l, "\"expected (string string)\" '%S", args);
        return match(symval(car(args)), symval(CADR(args))) ? gsym_tee() : gsym_nil(); 
}

static cell *subr_scons(lisp *l, cell *args) { /**< concatenate two strings */
        char *ret;
        /**@todo add support for prepending or appending integer-character*/
        if(!cklen(args, 2) 
        || !is_asciiz(car(args)) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (string string)\" '%S", args);
        ret = CONCATENATE(strval(car(args)), strval(CADR(args)));
        return mkstr(l, ret);
}

static cell *subr_scar(lisp *l, cell *args) { /**< get first character of string*/
        char c[2] = {'\0', '\0'};
        if(!cklen(args, 1) || !is_asciiz(car(args)))
                RECOVER(l, "\"expected (string-or-symbol)\" '%S", args);
        c[0] = strval(car(args))[0];
        return mkstr(l, lstrdup(c));
}


static cell *subr_scdr(lisp *l, cell *args) { /**< get all but first character of string*/
        if(!cklen(args, 1) || !is_asciiz(car(args)))
                RECOVER(l, "\"expected (string-or-symbol)\" '%S", args);
        if(!(strval(car(args))[0])) mkstr(l, lstrdup(""));
        return mkstr(l, lstrdup(&strval(car(args))[1]));;
}

static cell *subr_eval(lisp *l, cell *args) { /**< evaluate a lisp expression*/
        cell *ob = NULL;
        int restore_used, r;
        jmp_buf restore;
        if(l->recover_init) {
                memcpy(restore, l->recover, sizeof(jmp_buf));
                restore_used = 1;
        }
        l->recover_init = 1;
        if((r = setjmp(l->recover))) {
                RECOVER_RESTORE(restore_used, l, restore); 
                return gsym_error();
        }

        if(cklen(args, 1)) ob = eval(l, l->cur_depth, car(args), l->top_env);
        if(cklen(args, 2)) {
                if(!is_cons(CADR(args)))
                        RECOVER(l, "\"expected a-list\" '%S", args);
                ob = eval(l, l->cur_depth, car(args), CADR(args));
        }

        RECOVER_RESTORE(restore_used, l, restore); 
        if(!ob) RECOVER(l, "\"expected (expr) or (expr environment)\" '%S", args);
        return ob;
}

static cell *subr_trace_level(lisp *l, cell *args) { /**< set global tracing level*/
        if(cklen(args, 1)) {
                if(is_int(car(args))) {
                        switch(intval(car(args))) {
                        case TRACE_OFF:    l->trace = TRACE_OFF; break;
                        case TRACE_MARKED: l->trace = TRACE_MARKED; break;
                        case TRACE_ALL:    l->trace = TRACE_ALL; break;
                        default: RECOVER(l, "\"invalid trace level\" '%S", car(args));
                        }
                } 
                else RECOVER(l, "\"expected (int)\" '%S", args);
        }
        return mkint(l, l->trace);
}

static cell *subr_trace_cell(lisp *l, cell *args) { /**< trace a specific cell if level allows*/
        if(cklen(args, 1)) {
                return (car(args)->trace) ? gsym_tee() : gsym_nil();
        } else if (cklen(args, 2)) {
                if(is_nil(CADR(args))) {
                        car(args)->trace = 0;
                        return gsym_nil();
                } else if(CADR(args) == gsym_tee()) {
                        car(args)->trace = 1;
                        return gsym_tee();
                } 
        } 
        RECOVER(l, "\"expected (cell) or (cell t-or-nil)\", '%S", args);
        return gsym_error();
}

static cell *subr_gc(lisp *l, cell *args) { /**< control the garbage collector*/
        if(cklen(args, 0))
                gc_mark_and_sweep(l);
        if(cklen(args, 1) && is_int(car(args))) {
                switch(intval(car(args))) {
                case GC_ON:       if(l->gc_state == GC_OFF) goto fail;
                                  else l->gc_state = GC_ON;
                                  break;
                case GC_POSTPONE: if(l->gc_state == GC_OFF) goto fail;
                                  else l->gc_state = GC_POSTPONE; 
                                  break;
                case GC_OFF:      l->gc_state = GC_OFF;      break;
                default: RECOVER(l, "\"invalid GC option\" '%S", args);
                }
        }
        return mkint(l, l->gc_state);
fail:   RECOVER(l, "\"garbage collection permanently off\" '%S", args);
        return gsym_error();
}

static cell *subr_length(lisp *l, cell *args) { /**< length of list or string*/
        if(!cklen(args, 1)) RECOVER(l, "\"argument count is not 1\" '%S", args);
        return mkint(l, (intptr_t)car(args)->len);
}

static cell* subr_inp(lisp *l, cell *args) { /**< is input port?*/
        if(!cklen(args, 1)) RECOVER(l, "\"argument count is not 1\" '%S", args);
        return is_in(car(args)) ? gsym_tee() : gsym_nil();
}

static cell* subr_outp(lisp *l, cell *args) { /**< is output port?*/
        if(!cklen(args, 1)) RECOVER(l, "\"argument count is not 1\" '%S", args);
        return is_out(car(args)) ? gsym_tee() : gsym_nil();
}

static cell* subr_open(lisp *l, cell *args) { /**< open a port*/
        io *ret = NULL;
        char *file;
        if(!cklen(args, 2) || !is_int(car(args)) || !is_asciiz(CADR(args))) 
                RECOVER(l, "\"expected (integer string)\" '%S", args);
        file = strval(CADR(args));
        switch(intval(car(args))) {
        case FIN:  ret = io_fin(fopen(file, "rb")); break;
        case FOUT: ret = io_fout(fopen(file, "wb")); break;
        case SIN:  ret = io_sin(file); break;
        /*case SOUT: will not be implemented.*/
        default:   RECOVER(l, "\"invalid operation %d\" '%S", intval(car(args)), args);
        }
        return ret == NULL ? gsym_nil() : mkio(l, ret);
}

static cell* subr_getchar(lisp *l, cell *args) { /**< get a character*/
        if(cklen(args, 0)) return mkint(l, io_getc(l->ifp));
        if(cklen(args, 1) && is_in(car(args)))
                return mkint(l, io_getc(ioval(car(args))));
        RECOVER(l, "\"expected () or (input)\" '%S", args);
        return gsym_error();
}

static cell* subr_getdelim(lisp *l, cell *args) {
        int ch;
        char *s; 
        if(cklen(args, 1) && (is_asciiz(car(args)) || is_int(car(args)))) {
                ch = is_asciiz(car(args)) ? strval(car(args))[0] : intval(car(args));
                return (s = io_getdelim(l->ifp, ch)) ? mkstr(l, s) : gsym_nil();
        }
        if(cklen(args, 2) && is_in(car(args)) && (is_asciiz(CADR(args)) || is_int(CADR(args)))) {
                ch = is_asciiz(CADR(args)) ? strval(CADR(args))[0] : intval(CADR(args));
                return (s = io_getdelim(ioval(car(args)), ch)) ? mkstr(l, s) : gsym_nil();
        }
        RECOVER(l, "\"expected (string) or (input string)\" '%S", args);
        return gsym_error();
}

static cell* subr_read(lisp *l, cell *args) { /**< read in an S-Expression*/
        cell *ob = NULL;
        int restore_used, r;
        jmp_buf restore;
        char *s = NULL;
        if(l->recover_init) {
                memcpy(restore, l->recover, sizeof(jmp_buf));
                restore_used = 1;
        }
        l->recover_init = 1;
        if((r = setjmp(l->recover))) { 
                RECOVER_RESTORE(restore_used, l, restore); 
                return gsym_error();
        }

        if(cklen(args, 0))
                ob = (ob = reader(l, l->ifp)) ? ob : gsym_error();
        if(cklen(args, 1) && (is_in(car(args)) || is_str(car(args)))) {
                io *i = NULL;
                if(is_in(car(args))){
                        i = ioval(car(args));
                } else {
                        s = strval(car(args));
                        if(!(i = io_sin(s)))
                                HALT(l, "\"%s\"", "out of memory");
                }
                ob = (ob = reader(l, i)) ? ob : gsym_error();
                if(s) io_close(i);

        }
        RECOVER_RESTORE(restore_used, l, restore); 
        if(!ob) RECOVER(l, "\"expected () or (input)\" '%S", args);
        return ob;
}

static cell* subr_puts(lisp *l, cell *args) { /**< print a string (raw)*/
        if(cklen(args, 1) && is_asciiz(car(args)))
                return io_puts(strval(car(args)),l->ofp) < 0 ? gsym_nil() : car(args);
        if(cklen(args, 2) && is_out(car(args)) && is_asciiz(CADR(args)))
                return io_puts(strval(CADR(args)), ioval(car(args))) < 0 ?
                        gsym_nil() : CADR(args);
        RECOVER(l, "\"expected (string) or (output string)\" '%S", args);
        return gsym_error();
}

static cell* subr_putchar(lisp *l, cell *args) { /**< print a single character*/
        if(cklen(args, 1) && is_int(car(args)))
                return io_putc(intval(car(args)),l->ofp) < 0 ? gsym_nil() : car(args);
        if(cklen(args, 2) && is_out(car(args)) && is_int(CADR(args)))
                return io_putc(intval(car(args)), ioval(CADR(args))) < 0 ?
                        gsym_nil() : CADR(args);
        RECOVER(l, "\"expected (integer) or (output integer)\" '%S", args);
        return gsym_error();
}

static cell* subr_print(lisp *l, cell *args) { /**< print out a single S-Expression*/
        if(cklen(args, 1)) 
                return printer(l, l->ofp, car(args), 0) < 0 ? gsym_nil() : car(args); 
        if(cklen(args, 2) && is_out(car(args))) 
                return printer(l, ioval(car(args)), CADR(args), 0) < 0 ? 
                        gsym_nil() : CADR(args); 
        RECOVER(l, "\"expected (expr) or (output expression)\" '%S", args);
        return gsym_error();
}

static cell* subr_flush(lisp *l, cell *args) { /**< flush an output port*/
        if(cklen(args, 0)) return mkint(l, fflush(NULL));
        if(cklen(args, 1) && is_io(car(args))) 
                return io_flush(ioval(car(args))) ? gsym_nil() : gsym_tee();
        RECOVER(l, "\"expected () or (io)\" '%S", args);
        return gsym_error();
}

static cell* subr_tell(lisp *l, cell *args) { /**< get offset of port*/
        if(cklen(args, 1) && is_io(car(args)))
                return mkint(l, io_tell(ioval(car(args))));
        RECOVER(l, "\"expected (io)\" '%S", args);
        return gsym_error();
}

static cell* subr_seek(lisp *l, cell *args) { /**< set offset of port*/
        if(cklen(args, 3) && is_io(car(args)) 
                && is_int(CADR(args)) && is_int(CADR(cdr(args)))) {
                switch (intval(CADR(cdr(args)))) {
                case SEEK_SET: case SEEK_CUR: case SEEK_END: break;
                default: RECOVER(l, "\"invalid enum option\" '%S", args);
                }
                return mkint(l,io_seek(ioval(car(args)),intval(CADR(args)),
                                        intval(CADR(cdr(args)))));
        }
        RECOVER(l, "\"expected (io integer integer)\" '%S", args);
        return gsym_error();
}

static cell* subr_eofp(lisp *l, cell *args) { /**< is port at the end of the stream?*/
        if(cklen(args, 1) && is_io(car(args)))
                return io_eof(ioval(car(args))) ? gsym_tee() : gsym_nil();
        RECOVER(l, "\"expected (io)\" '%S", args);
        return gsym_error();
}

static cell* subr_ferror(lisp *l, cell *args) { /**< did an error occur on a port?*/
        if(cklen(args, 1) && is_io(car(args)))
                return io_error(ioval(car(args))) ? gsym_tee() : gsym_nil();
        RECOVER(l, "\"expected (io)\" '%S", args);
        return gsym_error();
}

static cell* subr_system(lisp *l, cell *args) { /**< execute command with system command interpreter*/
        if(cklen(args, 0))
                return mkint(l, system(NULL));
        if(cklen(args, 1) && is_asciiz(car(args)))
                return mkint(l, system(strval(car(args))));
        RECOVER(l, "\"expected () or (string)\" '%S", args);
        return gsym_error();
}

static cell* subr_remove(lisp *l, cell *args) {
        if(!cklen(args, 1) || !is_asciiz(car(args)))
                RECOVER(l, "\"expected (string)\" '%S", args);
        return remove(strval(car(args))) ? gsym_nil() : gsym_tee() ;
}

static cell* subr_rename(lisp *l, cell *args) { /**< rename a file in file system*/
        if(!cklen(args, 2) 
        || !is_asciiz(car(args)) || !is_asciiz(CADR(args))) 
                RECOVER(l, "\"expected (string string)\" '%S", args);
        return rename(strval(car(args)), strval(CADR(args))) ? gsym_nil() : gsym_tee();
}

static cell* subr_hlookup(lisp *l, cell *args) { /**< lookup key in hash*/
        cell *ob;
        if(!cklen(args, 2) || !is_hash(car(args)) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (hash symbol-or-string)\" %S", args);
        return (ob = hash_lookup(hashval(car(args)),
                                symval(CADR(args)))) ? ob : gsym_nil(); 
}

static cell* subr_hinsert(lisp *l, cell *args) { /**< insert key into hash*/
        if(!cklen(args, 3) || !is_hash(car(args)) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (hash symbol expression)\" %S", args);
        if(hash_insert(hashval(car(args)), 
                        symval(CADR(args)), 
                        cons(l, CADR(args), CADR(cdr(args)))))
                                HALT(l, "%s", "out of memory");
        return car(args); 
}

static cell* subr_hcreate(lisp *l, cell *args) { /**< create a new hash*/
        hashtable *ht;
        if(args->len % 2)
                RECOVER(l, "\"expected even number of arguments\" '%S", args);
        if(!(ht = hash_create(DEFAULT_LEN))) HALT(l, "%s", "out of memory");
        for(;!is_nil(args); args = cdr(cdr(args))) {
                if(!is_asciiz(car(args))) return gsym_error();
                hash_insert(ht, symval(car(args)), cons(l, car(args), CADR(args)));
        }
        return mkhash(l, ht); 
}

static cell* subr_coerce(lisp *l, cell *args) { /**< coerce one type into another*/
        char *fltend = NULL;
        intptr_t d = 0;
        size_t i = 0, j;
        cell *convfrom, *x, *y, *head;
        if(!cklen(args, 2) && is_sym(car(args))) goto fail;
        convfrom = CADR(args);
        if(intval(car(args)) == convfrom->type) return convfrom;
        switch(intval(car(args))) {
        case INTEGER: 
                    if(is_str(convfrom)) { /*int to string*/
                            if(!is_number(strval(convfrom))) goto fail;
                            sscanf(strval(convfrom), "%"SCNiPTR, &d);
                    }
                    if(is_floatval(convfrom)) /*float to string*/
                           d = (intptr_t) floatval(convfrom);
                    return mkint(l, d);
        case CONS:  if(is_str(convfrom)) { /*string to list of chars*/
                            head = x = cons(l, gsym_nil(), gsym_nil());
                            for(i = 0; i < convfrom->len; i++) {
                                char c[2] = {'\0', '\0'};
                                c[0] = strval(convfrom)[i];
                                y = mkstr(l, lstrdup(c));
                                setcdr(x, cons(l, y, gsym_nil()));
                                x = cdr(x);
                            }
                            cdr(head)->len = i;
                            return cdr(head);
                    }
                    if(is_hash(convfrom)) { /*hash to list*/
                            hashentry *cur;
                            hashtable *h = hashval(convfrom);
                            head = x = cons(l, gsym_nil(), gsym_nil());
                            for(j = 0, i = 0; i < h->len; i++)
                                    if(h->table[i])
                                            for(cur = h->table[i]; cur; cur = cur->next, j++) {
                                                    y = mkstr(l, lstrdup(cur->key));
                                                    setcdr(x, cons(l, y, gsym_nil()));
                                                    x = cdr(x);
                                                    setcdr(x, cons(l, (cell*)cur->val, gsym_nil()));
                                                    x = cdr(x);
                                            }
                            cdr(head)->len = j;
                            return cdr(head);
                    }
                    break;
        case STRING:if(is_int(convfrom)) { /*int to string*/
                            char s[64] = "";
                            sprintf(s, "%"PRIiPTR, intval(convfrom));
                            return mkstr(l, lstrdup(s));
                    }
                    if(is_sym(convfrom)) /*symbol to string*/
                           return mkstr(l, lstrdup(strval(convfrom)));
                    if(is_floatval(convfrom)) { /*float to string*/
                            char s[512] = "";
                            sprintf(s, "%f", floatval(convfrom));
                            return mkstr(l, lstrdup(s));
                    }
                    if(is_cons(convfrom)) { /*list of chars/ints to string*/
                        /**@bug implement me*/
                    }
                    break;
        case SYMBOL:if(is_str(convfrom) && !strpbrk(strval(convfrom), " ;#()\t\n\r'\"\\"))
                            return intern(l, lstrdup(strval(convfrom)));
                    break;
        case HASH:  if(is_cons(convfrom)) /*hash from list*/
                            return subr_hcreate(l, convfrom);
                    break;
        case FLOAT: if(is_int(convfrom)) /*int to float*/
                          return mkfloat(l, intval(convfrom));
                    if(is_str(convfrom)) { /*string to float*/
                          lfloat d;
                          if(!is_fnumber(strval(convfrom))) goto fail;
                          d = strtod(strval(convfrom), &fltend);
                          if(!fltend[0]) return mkfloat(l, d);
                          else           goto fail;
                    }
        default: break;
        }
fail:   RECOVER(l, "\"invalid conversion or argument length not 2\" %S", args);
        return gsym_error();
}

static cell* subr_time(lisp *l, cell *args) { /**< get the time*/
        if(!cklen(args, 0))
                RECOVER(l, "\"expected ()\" %S", args);
        return mkint(l, time(NULL));
}

static cell* subr_date(lisp *l, cell *args) { /**< get the current date*/
        time_t raw;
        struct tm *gt; 
        /*@warning This function is not thread safe, also only it 
         *         only provided GMT time*/
        if(!cklen(args, 0))
                RECOVER(l, "\"expected ()\" %S", args);
        time(&raw);
        gt = gmtime(&raw);
        return cons(l,  mkint(l, gt->tm_year + 1900),
                cons(l, mkint(l, gt->tm_mon + 1),
                cons(l, mkint(l, gt->tm_mday),
                cons(l, mkint(l, gt->tm_hour),
                cons(l, mkint(l, gt->tm_min),
                cons(l, mkint(l, gt->tm_sec), gsym_nil()))))));
}

static cell* subr_getenv(lisp *l, cell *args) { /**< get an environment string*/
        char *ret;
        if(!cklen(args, 1) || !is_asciiz(car(args)))
                RECOVER(l, "\"expected (string)\" '%S", args);
        return (ret = getenv(strval(car(args)))) ? mkstr(l, lstrdup(ret)) : gsym_nil();
}

static cell *subr_rand(lisp *l, cell *args) { /**< get a (pseudo) random number*/
        if(!cklen(args, 0))
                RECOVER(l, "\"expected ()\" %S", args);
        return mkint(l, xorshift128plus(l->random_state));
}

static cell *subr_seed(lisp *l, cell *args) { /**< seed the PRNG*/
        if(!cklen(args, 2) || !is_int(car(args)) || !is_int(CADR(args)))
                RECOVER(l, "\"expected (integer integer)\" %S", args);
        l->random_state[0] = intval(car(args));
        l->random_state[1] = intval(CADR(args));
        return gsym_tee();
}

static cell* subr_assoc(lisp *l, cell *args) { /**< associate a value with a key*/
        if(!cklen(args, 2) || !is_cons(CADR(args)))
                RECOVER(l, "\"expected (val a-list)\" '%S", args);
        return assoc(car(args), CADR(args));
}

static cell *subr_setlocale(lisp *l, cell *args) { /**< set locale*/
        char *ret = NULL; 
        /** @warning this function affect the global state of the program*/
        if(!cklen(args, 2) || !is_int(car(args)) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (int string-or-symbol)\" '%S", args);
        switch(intval(car(args))) {
        case LC_ALL:      case LC_COLLATE: case LC_CTYPE:
        case LC_MONETARY: case LC_NUMERIC: case LC_TIME:
                ret = setlocale(intval(car(args)), strval(CADR(args)));
                break;
        default: RECOVER(l, "\"invalid int value\" '%S", args);
        }
        if(!ret) return gsym_nil(); /*failed to set local*/
        return mkstr(l, lstrdup(ret));
}

static cell *subr_typeof(lisp *l, cell *args) { /**< get type specifier of argument*/
        if(!cklen(args, 1))
                RECOVER(l, "\"expected (expr)\" %S", args);
        return mkint(l, car(args)->type);
}

static cell *subr_close(lisp *l, cell *args) { /**< close port*/
        cell *x;
        if(!cklen(args, 1) || !is_io(car(args)))
                RECOVER(l, "\"expected (io)\" %S", args);
        x = car(args);
        x->close = 1;
        io_close(ioval(x));
        return x;
}

static cell *subr_timed_eval(lisp *l, cell *args) { /**< time an evaluation */
        clock_t start, end;
        lfloat used;
        cell *x;
        start = clock();
        x = subr_eval(l, args);
        end = clock();
        used = ((lfloat) (end - start)) / CLOCKS_PER_SEC;
        return cons(l, mkfloat(l, used), x);
}

static cell *subr_reverse(lisp *l, cell *args) { /**< reverse an object*/
        if(!cklen(args, 1)) goto fail;
        if(gsym_nil() == car(args)) return gsym_nil();
        switch(car(args)->type) {
        case STRING:
                {       
                        char *s = lstrdup(strval(car(args))), c;
                        size_t i = 0, len;
                        if(!s) HALT(l, "\"%s\"", "out of memory");
                        if(!(car(args)->len))
                                return mkstr(l, s);
                        len = car(args)->len - 1;
                        do {
                                c = s[i];
                                s[i] = s[len - i];
                                s[len - i] = c;
                        } while(i++ < (len / 2));
                        return mkstr(l, s);
                } break;
        case CONS:
                {
                        cell *x = car(args), *y = gsym_nil();
                        if(!is_cons(cdr(x)) && !is_nil(cdr(x)))
                                return cons(l, cdr(x), car(x));
                        for(; !is_nil(x); x = cdr(x)) 
                                y = cons(l, car(x), y);
                        return y;
                } break;
        case HASH: /**@bug implement me*/
                break;
        default:   
                break;
        }
fail:   RECOVER(l, "\"expected () (string) (list) (hash)\" %S", args);
        return gsym_error();
}

static cell *subr_join(lisp *l, cell *args) { /**< join a list of strings together*/
        /**@todo add support for joining integers as character strings*/
        char *sep = "", *r, *tmp;
        if(args->len < 2 || !is_asciiz(car(args))) 
                goto fail;
        sep = strval(car(args));
        if(!is_asciiz(CADR(args))) {
                if(is_cons(CADR(args))) {
                        args = CADR(args);
                        if(!is_asciiz(car(args)))
                                goto fail;
                } else {
                        goto fail;
                }
        } else {
                args = cdr(args);
        }
        r = lstrdup(strval(car(args)));
        for(args = cdr(args); !is_nil(args); args = cdr(args)) {
                if(!is_asciiz(car(args))) {
                        free(r);
                        goto fail;
                }
                tmp = vstrcatsep(sep, r, strval(car(args)), NULL);
                free(r);
                r = tmp;
        }
        return mkstr(l, r);
fail:   RECOVER(l, "\"expected (string string...) or (string (string ...))\" %S", args);
        return gsym_error();
}

static cell *subr_regexspan(lisp *l, cell *args) { /**< get the span of a regex match*/
        regex_result rr;
        cell *m = gsym_nil();
        if(!cklen(args, 2) || !is_asciiz(car(args)) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (string string)\" %S", args);
        rr = regex_match(strval(car(args)), strval(CADR(args)));
        if(rr.result <= 0)
                rr.start = rr.end = strval(CADR(args)) - 1;
        m = (rr.result < 0 ? gsym_error() : (rr.result == 0 ? gsym_nil() : gsym_tee()));
        return cons(l, m, 
                cons(l, mkint(l, rr.start - strval(CADR(args))),
                cons(l, mkint(l, rr.end   - strval(CADR(args))), gsym_nil())));
}

static cell *subr_raise(lisp *l, cell *args) { /**< raise a signal*/
        if(!cklen(args, 1) || !is_int(car(args)))
                RECOVER(l, "\"expected (integer)\" %S", args);
        return raise(intval(car(args))) ? (cell*)gsym_nil(): (cell*)gsym_tee();
}

static cell *subr_split(lisp *l, cell *args) { /**< split a string based on a regex*/
        char *pat, *s, *f;
        cell *op = gsym_nil(), *head;
        regex_result rr;
        if(!cklen(args, 2) || !is_asciiz(car(args)) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (string string)\" %S", args);
        pat = strval(car(args));
        if(!(f = s = lstrdup(strval(CADR(args))))) 
                HALT(l, "\"%s\"", "out of memory");
        head = op = cons(l, gsym_nil(), gsym_nil());
        for(;;) {
                rr = regex_match(pat, s);
                if(!rr.result || rr.end == rr.start) {
                        setcdr(op, cons(l, mkstr(l, lstrdup(s)), gsym_nil()));
                        break;
                }
                rr.start[0] = '\0';
                setcdr(op, cons(l, mkstr(l, lstrdup(s)), gsym_nil()));
                op = cdr(op);
                s = rr.end;
        }
        free(f);
        return cdr(head);
}

static cell *subr_substring(lisp *l, cell *args) { /**< get a subset of a string*/
        intptr_t min, max, tmp;
        char *subs;
        if(!(args->len == 2 || args->len == 3) || !is_asciiz(car(args)) || !is_int(CADR(args)))
                RECOVER(l, "\"expected (string int int?)\" '%S", args);
        if(args->len == 3 && !is_int(CADDR(args)))
                RECOVER(l, "\"expected (string int int?)\" '%S", args);
        min = intval(CADR(args));
        if(args->len == 2) {
                if(min >= 0) {
                        min = MIN(min, car(args)->len);
                        assert(min < car(args)->len);
                        return mkstr(l, lstrdup(strval(car(args))+min));
                } else if (min < 0) {
                        min = car(args)->len + min;
                        min = MAX(0, min);
                        if(!(subs = calloc(min + 1, 1)))
                                HALT(l, "\"%s\"", "out of memory");
                        memcpy(subs, strval(car(args)) + min, car(args)->len - min);
                        return mkstr(l, subs);
                }
        }
        if(((max = intval(CADDR(args))) < 0) || min < 0)
                RECOVER(l, "\"substring lengths must positive for three arguments\" '%S", args);
        min = MIN(min, car(args)->len);
        if((min + max) >= car(args)->len) {
                tmp = (max + min) - car(args)->len;
                max = max - tmp;
                assert((min + max) <= car(args)->len);
        }
        if(!(subs = calloc(max + 1, 1)))
                HALT(l, "\"%s\"", "out of memory");
        memcpy(subs, strval(car(args)) + min, max);
        return mkstr(l, subs);
}

static cell *subr_format(lisp *l, cell *args) { /**<print out arguments based on format string*/
        cell *cret;
        io *o = NULL, *t;
        char *fmt, c;
        int ret = 0, pchar;
        if(cklen(args, 0)) return gsym_nil();
        if(is_out(car(args))) {
                o = ioval(car(args));
                args = cdr(args);
        } else {
                o = l->ofp;
        }
        if(!is_asciiz(car(args)))
                RECOVER(l, "\"expected () (io string expr...) (string expr...)\" '%S", args);
        if(!(t = io_sout(calloc(2, 1), 2)))
                HALT(l, "\"%s\"", "out of memory");
        fmt = strval(car(args));
        args = cdr(args);
        while((c = *fmt++))
                if(ret == EOF) goto fail;
                else if('%' == c) {
                        switch((c = *fmt++)) {
                        case '\0': goto fail;
                        case '%':  ret = io_putc(c, t); 
                                   break;
                        case 'c':  if(is_nil(args) || (!is_asciiz(car(args)) && !is_int(car(args))))
                                           goto fail;
                                   if(is_int(car(args))) {
                                           pchar = intval(car(args));
                                   } else { /*must be a character*/
                                        if(!cklen(car(args), 1))
                                                goto fail;
                                        pchar = strval(car(args))[0];
                                   }
                                   ret = io_putc(pchar, t);
                                   args = cdr(args);
                                   break;
                        case 's':  if(is_nil(args) || !is_asciiz(car(args)))
                                           goto fail;
                                   ret = io_puts(strval(car(args)), t);
                                   args = cdr(args);
                                   break;
                        case 'S':  if(is_nil(args))
                                           goto fail;
                                   ret = printer(l, t, car(args), 0); 
                                   args = cdr(args);
                                   break;
                        default:   goto fail;
                        }
                } else {
                        ret = io_putc(c, t);
                }
        if(!is_nil(args))
                goto fail;
        io_puts(t->p.str, o);
        cret = mkstr(l, t->p.str); /*t->p.str is not freed by io_close*/
        io_close(t);
        return cret;
fail:   free(t->p.str);
        io_close(t);
        RECOVER(l, "\"format error\" %S", args);
        return gsym_error();
#undef  RESTORE_IO_STATE
}

static cell *subr_tr(lisp *l, cell *args) { /**< translate characters*/
        /**@todo this function needs a lot of improvement, as does the
         *       functions that implement the translation routines*/
        tr_state st;
        char *mode, *s1, *s2, *tr, *ret; 
        size_t len, i;
        if(cklen(args, 4)) {
                cell *t = args;
                for(i = 0; !is_nil(t) && i < 4; i++, t = cdr(t)) 
                        if(!is_str(car(args))) 
                                goto fail;
        } else goto fail;
        mode = strval(car(args));
        s1   = strval(CADR(args));
        s2   = strval(CADDR(args));
        tr   = strval(CADDDR(args));
        len  = CADDDR(args)->len;
        memset(&st, 0, sizeof(st));
        switch(tr_init(&st, mode, (uint8_t*)s1, (uint8_t*)s2)) {
                case TR_OK:      break;
                case TR_EINVAL:  RECOVER(l, "\"invalid mode\" \"%s\"", mode);
                case TR_DELMODE: RECOVER(l, "\"set 2 not NULL in deleted mode\" '%S", args);
                default:         RECOVER(l, "\"unknown tr error\" '%S", args);
        }
        if(!(ret = calloc(len + 1, 1)))
                HALT(l, "\"%s\"", "out of memory");
        tr_block(&st, (uint8_t*)tr, (uint8_t*)ret, len);  
        return mkstr(l, ret);
fail:   RECOVER(l, "\"expected (string string string string)\" '%S", args);
        return gsym_error();
}

