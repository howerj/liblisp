/** @file       subr.c
 *  @brief      The liblisp interpreter built in subroutines
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *
 *  This file is the largest one in the project, this normally would be a
 *  problem, however all of the functions (apart from lisp_init and related"
 *  functions) follow the same pattern, apart from a very few exceptions they
 *  do not call each other. All of the functions named subr_X (X being the rest
 *  of the function name) abide by the same interface as they are built in
 *  subroutines for the lisp interpreter.
 *
 *  This file is uses X-Macros to populated tables of subroutines and variables
 *  along with their associated names.
 *
 *  While each function has a very short description about what it does the
 *  manual (which should be called something like "liblisp.md") describes
 *  what each subroutine primitive does within the context of the interpreter.
 *
 *  @todo Assertions should be added throughout these routines to make sure
 *        operations do not go out of bounds.
 *  @todo Remove as many variable length functions as possible.
 *  @todo Make the open/close routines more generic (so that they can deal
 *        with user defined types).
 *  @todo add interface to errno and more C-library stuff.
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

/* X-Macro of primitive functions and their names; basic built in subr
 * The format is:
 *       primitive subroutine
 *       argument length (only if the validate string is not NULL)
 *       validate string (if NULL it is not used)
 *       documentation string (if NULL then there is no doc-string)
 *       subroutine name (as it appears with in the interpreter) */
#define SUBROUTINE_XLIST\
  X("assoc",       subr_assoc,     "A c",  "lookup a variable in an 'a-list'")\
  X("blog",        subr_binlog,    "d",    "compute the binary logarithm of an integer")\
  X("ipow",        subr_ipow,      "d d",  "compute the integer exponentiation of two numbers")\
  X("car",         subr_car,       "c",    "return the first object in a list")\
  X("cdr",         subr_cdr,       "c",    "return every object apart from the first in a list")\
  X("close",       subr_close,     "P",    "close a port, invalidating it")\
  X("closed?",     subr_is_closed, NULL,   "is a object closed?")\
  X("coerce",      subr_coerce,    NULL,   "coerce a variable from one type to another")\
  X("cons",        subr_cons,      "A A",  "allocate a new cons cell with two arguments")\
  X("date",        subr_date,      "",     "return a list representing the date (GMT) (not thread safe)")\
  X("define-eval", subr_define_eval, "s A", "extend the top level environment with a computed symbol")\
  X("documentation-string", subr_doc_string, "x", "return the documentation string from a procedure")\
  X("eof?",        subr_eofp,      "P",    "is the EOF flag set on a port?")\
  X("eq",          subr_eq,        "A A",  "equality operation")\
  X("eval",        subr_eval,      NULL,   "evaluate an expression")\
  X("ferror",      subr_ferror,    "P",    "is the error flag set on a port")\
  X("flush",       subr_flush,     NULL,   "flush a port")\
  X("format",      subr_format,    NULL,   "print a string given a format and arguments")\
  X("validation-string", subr_validation_string, "x", "return the format string from a procedure")\
  X("gc",          subr_gc,        "",     "force the collection of garbage")\
  X("get-char",    subr_getchar,   "i",    "read in a character from a port")\
  X("get-delim",   subr_getdelim,  "i C",  "read in a string delimited by a character from a port")\
  X("getenv",      subr_getenv,    "Z",    "get an environment variable (not thread safe)")\
  X("hash-create", subr_hcreate,   NULL,   "create a new hash")\
  X("hash-insert", subr_hinsert,   "h Z A", "insert a variable into a hash")\
  X("hash-lookup", subr_hlookup,   "h Z",  "loop up a variable in a hash")\
  X("input?",      subr_inp,       "A",    "is an object an input port?")\
  X("join",        subr_join,      NULL,   "join a list of strings together with a separator")\
  X("length",      subr_length,    "A",    "return the length of a list or string")\
  X("list",        subr_list,      NULL,   "create a list from the arguments")\
  X("locale!",     subr_setlocale, "d Z",  "set the locale, this affects global state!")\
  X("match",       subr_match,     "Z Z",  "perform a primitive match on a string")\
  X("open",        subr_open,      "d Z",  "open a port (either a file or a string) for reading *or* writing")\
  X("output?",     subr_outp,      "A",    "is an object an output port?")\
  X("print",       subr_print,     "o A",  "print out an s-expression")\
  X("procedure-args",  subr_proc_args, "l", "return the arguments from a user defined procedure")\
  X("procedure-code",  subr_proc_code, "l", "return the code from a user defined procedure")\
  X("put-char",    subr_putchar,   "o d",  "write a character to a output port")\
  X("put",         subr_puts,      "o Z",  "write a string to a output port")\
  X("raise",       subr_raise,     "d",    "raise a signal")\
  X("random",      subr_rand,      "",     "return a pseudo random number generator")\
  X("read",        subr_read,      "I",    "read in an s-expression from a port or a string")\
  X("regex-span",  subr_regexspan, "Z Z",  "get the span of a regex match on a string")\
  X("remove",      subr_remove,    "Z",    "remove a file")\
  X("rename",      subr_rename,    "Z Z",  "rename a file")\
  X("reverse",     subr_reverse,   NULL,   "reverse a string, list or hash")\
  X("scar",        subr_scar,      "Z",    "return the first character in a string")\
  X("scdr",        subr_scdr,      "Z",    "return a string excluding the first character")\
  X("scons",       subr_scons,     "Z Z",  "concatenate two string")\
  X("seed",        subr_seed,      "d d",  "seed the pseudo random number generator")\
  X("seek",        subr_seek,      "P d d", "perform a seek on a port (moving the port position indicator)")\
  X("split",       subr_split,     "Z Z",  "split a string given a regular expression")\
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
  X("system",      subr_system,    NULL,   "execute a command with the system command interpreter")\
  X("tell",        subr_tell,      "P",    "return the position indicator of a port")\
  X("timed-eval",  subr_timed_eval, NULL,  "time an evaluation")\
  X("time",        subr_time,      "",     "create a list representing the time")\
  X("trace!",      subr_trace,     "b",    "turn tracing on or off")\
  X("tr",          subr_tr,        "Z Z Z Z", "translate a string given a format and mode")\
  X("type-of",     subr_typeof,    "A",    "return an integer representing the type of an object")\
  X("top-environment", subr_top_env, "",   "return the top level environment")\
  X("validate",    subr_validate,  "d Z c", "validate an argument list against a format string")

#define X(NAME, SUBR, VALIDATION, DOCSTRING) static cell* SUBR (lisp *l, cell *args);
SUBROUTINE_XLIST /*function prototypes for all of the built-in subroutines*/
#undef X

#define X(NAME, SUBR, VALIDATION, DOCSTRING) { SUBR, NAME, VALIDATION, "(" NAME ") : " DOCSTRING },
static struct all_subroutines { subr p; const char *name, *validate, *docstring; } primitives[] = {
        SUBROUTINE_XLIST /*all of the subr functions*/
        {NULL, NULL, NULL, NULL} /*must be terminated with NULLs*/
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
        X("*user-defined*", USERDEF)      X("*eof*",         EOF)\
        X("*sig-abrt*",     SIGABRT)      X("*sig-fpe*",     SIGFPE)\
        X("*sig-ill*",      SIGILL)       X("*sig-int*",     SIGINT)\
        X("*sig-segv*",     SIGSEGV)      X("*sig-term*",    SIGTERM)

#define X(NAME, VAL) { NAME, VAL }, 
static struct integer_list { char *name; intptr_t val; } integers[] = {
        INTEGER_XLIST
        {NULL, 0}
};
#undef X

#define X(CNAME, LNAME) static cell _ ## CNAME = { SYMBOL, 0, 1, 0, 0, .p[0].v = LNAME};
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


#define X(FNAME, IGNORE) cell *gsym_ ## FNAME (void) { return FNAME ; }
CELL_XLIST /**< defines functions to get a lisp "cell" for the built in special symbols*/
#undef X

#define SUBR_ISX(NAME)\
static cell *subr_ ## NAME (lisp *l, cell *args) { UNUSED(l);\
        char *s, c;\
        if(is_int(car(args)))\
                return NAME (get_int(car(args))) ? gsym_tee() : gsym_nil();\
        s = get_str(car(args));\
        if(!s[0]) return gsym_nil();\
        while((c = *s++)) \
                if(! NAME (c))\
                        return gsym_nil();\
        return gsym_tee();\
}

#define ISX_LIST\
        X(isalnum,  "Is a string or integer composed of alphanumeric characters?")\
        X(isalpha,  "Is a string or integer composed of alphabetic characters?")\
        X(iscntrl,  "Is a string or integer composed of control characters?")\
        X(isdigit,  "Is a string or integer composed of digits?")\
        X(isgraph,  "Is a string or integer composed of printable characters (excluding space)?")\
        X(islower,  "Is a string or integer composed of lower case characters?")\
        X(isprint,  "Is a string or integer composed of printable characters?")\
        X(ispunct,  "Is a string or integer composed of punctuation characters?")\
        X(isspace,  "Is a string or integer composed of whitespace characters?")\
        X(isupper,  "Is a string or integer composed of upper case characters?")\
        X(isxdigit, "Is a string or integer composed of hexadecimal digits?")

#define X(FUNC, IGNORE) SUBR_ISX(FUNC)
ISX_LIST /*defines lisp subroutines for checking whether a string only contains a character class*/
#undef X

lisp *lisp_init(void) {
        lisp *l;
        unsigned i;
        if(!(l = calloc(1, sizeof(*l))))     goto fail;
        if(!(l->ifp = io_fin(stdin)))        goto fail;
        if(!(l->ofp = io_fout(stdout)))      goto fail;
        if(!(l->efp = io_fout(stderr)))      goto fail;

        if(!(l->buf = calloc(DEFAULT_LEN, 1))) goto fail;
        l->buf_allocated = DEFAULT_LEN;
        if(!(l->gc_stack = calloc(DEFAULT_LEN, sizeof(*l->gc_stack)))) 
                goto fail;
        l->gc_stack_allocated = DEFAULT_LEN;
        l->max_depth          = LARGE_DEFAULT_LEN; /**< max recursion depth*/

#define X(CNAME, LNAME) l-> CNAME = CNAME;
CELL_XLIST
#undef X

        l->random_state[0] = 0xCAFEBABE; /*Are these good seeds?*/
        l->random_state[1] = 0xDEADC0DE;
        for(i = 0; i < LARGE_DEFAULT_LEN; i++) /*discard first N numbers*/
                (void)xorshift128plus(l->random_state);

        /* The lisp init function is now ready to add built in subroutines
         * and other variables, the order in which is does this matters. */

        if(!(l->all_symbols = mk_hash(l, hash_create(LARGE_DEFAULT_LEN)))) 
                goto fail;
        if(!(l->top_env = cons(l, cons(l, gsym_nil(), gsym_nil()),gsym_nil())))
                goto fail;

        /* Special care has to be taken with the input and output objects
         * as they can change throughout the interpreters lifetime, they
         * should only be set by accessor functions*/
        if(!(l->input   = mk_io(l, l->ifp))) goto fail;
        if(!(l->output  = mk_io(l, l->ofp))) goto fail;
        if(!(l->logging = mk_io(l, l->efp))) goto fail;

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
                if(!lisp_intern(l, special_cells[i].internal))
                        goto fail;
                if(!extend_top(l, special_cells[i].internal, 
                                  special_cells[i].internal))
                        goto fail;
        }

        if(!lisp_add_cell(l, "pi", mk_float(l, 3.14159265358979323846))) 
                goto fail;
        if(!lisp_add_cell(l, "e",  mk_float(l, 2.71828182845904523536))) 
                goto fail;


        for(i = 0; integers[i].name; i++) /*add all integers*/
                if(!lisp_add_cell(l, integers[i].name, 
                                        mk_int(l, integers[i].val)))
                        goto fail;
        for(i = 0; primitives[i].p; i++) /*add all primitives*/
                if(!lisp_add_subr(l, 
                     primitives[i].name,     primitives[i].p, 
                     primitives[i].validate, primitives[i].docstring))
                        goto fail;

#define X(FUNC, DOCSTRING) if(!lisp_add_subr(l, # FUNC "?", subr_ ## FUNC, "C", "(" # FUNC "?) : " DOCSTRING)) goto fail;
ISX_LIST /*add all of the subroutines for string character class testing*/
#undef X

        return l;
fail:   lisp_destroy(l);
        return NULL;
}

static cell *subr_band(lisp *l, cell *args) {
        return mk_int(l, get_int(car(args)) & get_int(CADR(args)));
}

static cell *subr_bor(lisp *l, cell *args) {
        return mk_int(l, get_int(car(args)) | get_int(CADR(args)));
}

static cell *subr_bxor(lisp *l, cell *args) { 
        return mk_int(l, get_int(car(args)) ^ get_int(CADR(args)));
}

static cell *subr_binv(lisp *l, cell *args) {
        return mk_int(l, ~get_int(car(args)));
}

static cell *subr_binlog(lisp *l, cell *args) { 
        return mk_int(l, binlog(get_int(car(args))));
}

static cell *subr_ipow(lisp *l, cell *args) {
        return mk_int(l, ipow(get_int(car(args)), get_int(CADR(args))));
}

static cell *subr_sum(lisp *l, cell *args) { 
        cell *x = car(args), *y = CADR(args);
        if(is_int(x))
                return mk_int(l, get_int(x) + get_a2i(y));
        return mk_float(l, get_float(x) + get_a2f(y));
}

static cell *subr_sub(lisp *l, cell *args) {
        cell *x = car(args), *y = CADR(args);
        if(is_int(x))
                return mk_int(l, get_int(x) - get_a2i(y));
        return mk_float(l, get_float(x) - get_a2f(y));
}

static cell *subr_prod(lisp *l, cell *args) { 
        cell *x = car(args), *y = CADR(args);
        if(is_int(x))
                return mk_int(l, get_int(x) * get_a2i(y));
        return mk_float(l, get_float(x) * get_a2f(y));
}

static cell *subr_mod(lisp *l, cell *args) { 
        intptr_t dividend, divisor;
        dividend = get_int(car(args));
        divisor  = get_int(CADR(args));
        if(!divisor || (dividend == INTPTR_MIN && divisor == -1)) 
                RECOVER(l, "\"invalid divisor values\" '%S", args);
        return mk_int(l, dividend % divisor);
}

static cell *subr_div(lisp *l, cell *args) {
        lfloat dividend, divisor;
        if(is_int(car(args))) {
                intptr_t dividend, divisor;
                dividend = get_int(car(args));
                divisor = get_a2i(CADR(args));
                if(!divisor || (dividend == INTPTR_MIN && divisor == -1))
                        RECOVER(l, "\"invalid divisor values\" '%S", args);
                return mk_int(l, dividend / divisor);
        } 
        dividend = get_float(car(args));
        divisor = get_a2f(CADR(args));
        if(divisor == 0.)
                RECOVER(l, "\"division by zero in %S\"", args);
        return mk_float(l, dividend / divisor);
}

static cell *subr_greater(lisp *l, cell *args) { 
        cell *x, *y;
        if(!cklen(args, 2)) goto fail;
        x = car(args);
        y = CADR(args);
        if(is_arith(x) && is_arith(y))
                return  (is_floating(x) ? get_float(x) : get_int(x)) > 
                        (is_floating(y) ? get_float(y) : get_int(y)) ? gsym_tee() : gsym_nil();
        else if(is_asciiz(x) && is_asciiz(x))
                return (strcmp(get_str(x), get_str(y)) > 0) ? gsym_tee() : gsym_nil();
fail:   RECOVER(l, "\"expected (number number) or (string string)\" '%S", args);
        return gsym_error();
}

static cell *subr_less(lisp *l, cell *args) { 
        cell *x, *y;
        if(!cklen(args, 2)) goto fail;
        x = car(args);
        y = CADR(args);
        if(is_arith(x) && is_arith(y))
                return  (is_floating(x) ? get_float(x) : get_int(x)) < 
                        (is_floating(y) ? get_float(y) : get_int(y)) ? gsym_tee() : gsym_nil();
        else if(is_asciiz(x) && is_asciiz(x))
                return (strcmp(get_str(x), get_str(y)) < 0) ? gsym_tee() : gsym_nil();
fail:   RECOVER(l, "\"expected (number number) or (string string)\" '%S", args);
        return gsym_error();
}

static cell *subr_eq(lisp *l, cell *args) { 
        cell *x, *y;
        x = car(args);
        y = CADR(args);
        if(is_userdef(x) && l->ufuncs[get_user_type(x)].equal)
                return (l->ufuncs[get_user_type(x)].equal)(x, y) ? 
                            gsym_tee() : gsym_nil();
        if(get_int(x) == get_int(y))
                return gsym_tee();
        if(is_str(x) && is_str(y)) {
                if(!strcmp(get_str(x), get_str(y))) return gsym_tee();
                else return gsym_nil();
        }
        return gsym_nil();
}

static cell *subr_cons(lisp *l, cell *args) {
        return cons(l, car(args), CADR(args)); 
}

static cell *subr_car(lisp *l, cell *args) { UNUSED(l);
        return CAAR(args); 
}

static cell *subr_cdr(lisp *l, cell *args) { UNUSED(l);
        return CDAR(args); 
}

static cell *subr_list(lisp *l, cell *args) {
        size_t i;
        cell *op, *head;
        if(cklen(args, 0))
                return gsym_nil();
        op = car(args);
        args = cdr(args);
        head = op = cons(l, op, gsym_nil());
        for(i = 1; !is_nil(args); args = cdr(args), op = cdr(op), i++)
                set_cdr(op, cons(l, car(args), gsym_nil()));
        head->len = i;
        return head;
}

static cell *subr_match(lisp *l, cell *args) { UNUSED(l);
        return match(get_sym(car(args)), get_sym(CADR(args))) ? 
                    gsym_tee() : gsym_nil(); 
}

static cell *subr_scons(lisp *l, cell *args) {
        char *ret;
        ret = CONCATENATE(get_str(car(args)), get_str(CADR(args)));
        return mk_str(l, ret);
}

static cell *subr_scar(lisp *l, cell *args) {
        char c[2] = {'\0', '\0'};
        c[0] = get_str(car(args))[0];
        return mk_str(l, lstrdup(c));
}

static cell *subr_scdr(lisp *l, cell *args) {
        if(!(get_str(car(args))[0])) mk_str(l, lstrdup(""));
        return mk_str(l, lstrdup(&get_str(car(args))[1]));;
}

static cell *subr_eval(lisp *l, cell *args) { 
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
        if(!ob) 
           RECOVER(l, "\"expected (expr) or (expr environment)\" '%S", args);
        return ob;
}

static cell *subr_trace(lisp *l, cell *args) { 
        l->trace = is_nil(car(args)) ? 0 : 1;
        return l->trace ? gsym_tee() : gsym_nil();
}

static cell *subr_gc(lisp *l, cell *args) { UNUSED(args);
        gc_mark_and_sweep(l);
        return gsym_tee();
}

static cell *subr_length(lisp *l, cell *args) {
        return mk_int(l, (intptr_t)get_length(car(args)));
}

static cell* subr_inp(lisp *l, cell *args) { UNUSED(l);
        return is_in(car(args)) ? gsym_tee() : gsym_nil();
}

static cell* subr_outp(lisp *l, cell *args) { UNUSED(l);
        return is_out(car(args)) ? gsym_tee() : gsym_nil();
}

static cell* subr_open(lisp *l, cell *args) {
        io *ret = NULL;
        char *file;
        file = get_str(CADR(args));
        switch(get_int(car(args))) {
        case FIN:  ret = io_fin(fopen(file, "rb")); break;
        case FOUT: ret = io_fout(fopen(file, "wb")); break;
        case SIN:  ret = io_sin(file); break;
      /*case SOUT: will not be implemented.*/
        default:   
          RECOVER(l, "\"invalid operation %d\" '%S", get_int(car(args)), args);
        }
        return ret == NULL ? gsym_nil() : mk_io(l, ret);
}

static cell* subr_getchar(lisp *l, cell *args) { 
        return mk_int(l, io_getc(get_io(car(args))));
}

static cell* subr_getdelim(lisp *l, cell *args) {
        int ch;
        char *s; 
        ch = is_asciiz(CADR(args)) ? get_str(CADR(args))[0] : get_int(CADR(args));
        return (s = io_getdelim(get_io(car(args)), ch)) ? mk_str(l, s) : gsym_nil();
}

static cell* subr_read(lisp *l, cell *args) { 
        cell *ob;
        int restore_used, r;
        jmp_buf restore;
        char *s;
        if(l->recover_init) { /*store exception state*/
                memcpy(restore, l->recover, sizeof(jmp_buf));
                restore_used = 1;
        }
        l->recover_init = 1;
        if((r = setjmp(l->recover))) { /*handle exception in reader*/
                RECOVER_RESTORE(restore_used, l, restore); 
                return gsym_error();
        }
        s = NULL;
        ob = NULL;
        io *i = NULL;
        if(!(i = is_in(car(args)) ? get_io(car(args)) : io_sin(get_str(car(args)))))
                HALT(l, "\"%s\"", "out of memory");
        ob = (ob = reader(l, i)) ? ob : gsym_error();
        if(s) io_close(i);
        RECOVER_RESTORE(restore_used, l, restore); 
        return ob;
}

static cell* subr_puts(lisp *l, cell *args) { UNUSED(l);
        return io_puts(get_str(CADR(args)), get_io(car(args))) < 0 ? 
                        gsym_nil() : CADR(args);
}

static cell* subr_putchar(lisp *l, cell *args) { UNUSED(l);
        /**@todo should put strings as well*/
        return io_putc(get_int(CADR(args)), get_io(car(args))) < 0 ?
                        gsym_nil() : CADR(args);
}

static cell* subr_print(lisp *l, cell *args) { 
        return printer(l, get_io(car(args)), CADR(args), 0) < 0 ? 
                gsym_nil() : CADR(args); 
}

static cell* subr_flush(lisp *l, cell *args) {
        if(cklen(args, 0)) return mk_int(l, fflush(NULL));
        if(cklen(args, 1) && is_io(car(args))) 
                return io_flush(get_io(car(args))) ? gsym_nil() : gsym_tee();
        RECOVER(l, "\"expected () or (io)\" '%S", args);
        return gsym_error();
}

static cell* subr_tell(lisp *l, cell *args) { 
        return mk_int(l, io_tell(get_io(car(args))));
}

static cell* subr_seek(lisp *l, cell *args) {
        switch (get_int(CADR(cdr(args)))) {
        case SEEK_SET: case SEEK_CUR: case SEEK_END: break;
        default: RECOVER(l, "\"invalid enum option\" '%S", args);
        }
        return mk_int(l, io_seek(get_io(car(args)), get_int(CADR(args)), get_int(CADR(cdr(args)))));
}

static cell* subr_eofp(lisp *l, cell *args) { UNUSED(l);
        return io_eof(get_io(car(args))) ? gsym_tee() : gsym_nil();
}

static cell* subr_ferror(lisp *l, cell *args) { UNUSED(l);
        return io_error(get_io(car(args))) ? gsym_tee() : gsym_nil();
}

static cell* subr_system(lisp *l, cell *args) { 
        if(cklen(args, 0))
                return mk_int(l, system(NULL));
        if(cklen(args, 1) && is_asciiz(car(args)))
                return mk_int(l, system(get_str(car(args))));
        RECOVER(l, "\"expected () or (string)\" '%S", args);
        return gsym_error();
}

static cell* subr_remove(lisp *l, cell *args) { UNUSED(l);
        return remove(get_str(car(args))) ? 
                     gsym_nil() : gsym_tee() ;
}

static cell* subr_rename(lisp *l, cell *args) { UNUSED(l);
        return rename(get_str(car(args)), get_str(CADR(args))) ? 
                     gsym_nil() : gsym_tee();
}

static cell* subr_hlookup(lisp *l, cell *args) { UNUSED(l);
        cell *ob;
        return (ob = hash_lookup(get_hash(car(args)),
                                get_sym(CADR(args)))) ? ob : gsym_nil(); 
}

static cell* subr_hinsert(lisp *l, cell *args) {
        if(hash_insert(get_hash(car(args)), 
                        get_sym(CADR(args)), 
                        cons(l, CADR(args), CADR(cdr(args)))))
                                HALT(l, "%s", "out of memory");
        return car(args); 
}

static cell* subr_hcreate(lisp *l, cell *args) { 
        hashtable *ht;
        if(get_length(args) % 2)
                RECOVER(l, "\"expected even number of arguments\" '%S", args);
        if(!(ht = hash_create(DEFAULT_LEN))) HALT(l, "%s", "out of memory");
        for(;!is_nil(args); args = cdr(cdr(args))) {
                if(!is_asciiz(car(args))) return gsym_error();
                hash_insert(ht, get_sym(car(args)), 
                                cons(l, car(args), CADR(args)));
        }
        return mk_hash(l, ht); 
}

static cell* subr_coerce(lisp *l, cell *args) { 
        char *fltend = NULL;
        intptr_t d = 0;
        size_t i = 0, j;
        cell *from, *x, *y, *head;
        if(!cklen(args, 2) && !is_int(car(args))) goto fail;
        from = CADR(args);
        if(get_int(car(args)) == from->type) return from;
        switch(get_int(car(args))) {
        case INTEGER: 
                    if(is_str(from)) { 
                            if(!is_number(get_str(from))) goto fail;
                            sscanf(get_str(from), "%"SCNiPTR, &d);
                    }
                    if(is_floating(from)) /*float to string*/
                           d = (intptr_t) get_float(from);
                    return mk_int(l, d);
        case CONS:  if(is_str(from)) { /*string to list of chars*/
                            head = x = cons(l, gsym_nil(), gsym_nil());
                            for(i = 0; i < from->len; i++) {
                                char c[2] = {'\0', '\0'};
                                c[0] = get_str(from)[i];
                                y = mk_str(l, lstrdup(c));
                                set_cdr(x, cons(l, y, gsym_nil()));
                                x = cdr(x);
                            }
                            cdr(head)->len = i;
                            return cdr(head);
                    }
                    if(is_hash(from)) { /*hash to list*/
                      hashentry *cur;
                      hashtable *h = get_hash(from);
                      head = x = cons(l, gsym_nil(), gsym_nil());
                      for(j = 0, i = 0; i < h->len; i++)
                        if(h->table[i])
                          for(cur = h->table[i]; cur; cur = cur->next, j++) {
                            y = mk_str(l, lstrdup(cur->key));
                            set_cdr(x, cons(l, y, gsym_nil()));
                            x = cdr(x);
                            set_cdr(x, cons(l, (cell*)cur->val, gsym_nil()));
                            x = cdr(x);
                                            }
                            cdr(head)->len = j;
                            return cdr(head);
                    }
                    break;
        case STRING:if(is_int(from)) { /*int to string*/
                            char s[64] = ""; /*holds all integer strings*/
                            sprintf(s, "%"PRIiPTR, get_int(from));
                            return mk_str(l, lstrdup(s));
                    }
                    if(is_sym(from)) /*symbol to string*/
                           return mk_str(l, lstrdup(get_str(from)));
                    if(is_floating(from)) { /*float to string*/
                            char s[512] = ""; /*holds all float strings*/
                            sprintf(s, "%f", get_float(from));
                            return mk_str(l, lstrdup(s));
                    }
                    if(is_cons(from)) { /*list of chars/ints to string*/
                            size_t i;
                            char *s;
                            for(x = from; !is_nil(x); x = cdr(x)) { /*validate args*/
                                    if(!is_proper_cons(x))
                                            goto fail; /*not a proper list*/
                                    if(!is_int(car(x)))
                                            goto fail; /*convert only integers*/
                            }
                            x = from;
                            if(!(s = calloc(get_length(x)+1, 1)))
                                    HALT(l, "\"%s\"", "out of memory");
                            for(i = 0; !is_nil(x); x = cdr(x), i++) 
                                    s[i] = get_int(car(x));
                            return mk_str(l, s);
                    }
                    break;
        case SYMBOL: if(is_str(from)) 
                            if(!strpbrk(get_str(from), " ;#()\t\n\r'\"\\"))
                                    return intern(l, lstrdup(get_str(from)));
                    break;
        case HASH:  if(is_cons(from)) /*hash from list*/
                            return subr_hcreate(l, from);
                    break;
        case FLOAT: if(is_int(from)) /*int to float*/
                          return mk_float(l, get_int(from));
                    if(is_str(from)) { /*string to float*/
                          lfloat d;
                          if(!is_fnumber(get_str(from))) goto fail;
                          d = strtod(get_str(from), &fltend);
                          if(!fltend[0]) return mk_float(l, d);
                          else           goto fail;
                    }
        default: break;
        }
fail:   RECOVER(l, "\"invalid conversion or argument length not 2\" %S", args);
        return gsym_error();
}

static cell* subr_time(lisp *l, cell *args) {  UNUSED(args);
        return mk_int(l, time(NULL));
}

static cell* subr_date(lisp *l, cell *args) { UNUSED(args);
        time_t raw;
        struct tm *gt; 
        time(&raw);
        gt = gmtime(&raw);
        return cons(l,  mk_int(l, gt->tm_year + 1900),
                cons(l, mk_int(l, gt->tm_mon),
                cons(l, mk_int(l, gt->tm_wday),
                cons(l, mk_int(l, gt->tm_mday),
                cons(l, mk_int(l, gt->tm_hour),
                cons(l, mk_int(l, gt->tm_min),
                cons(l, mk_int(l, gt->tm_sec), gsym_nil())))))));
}

static cell* subr_getenv(lisp *l, cell *args) { 
        char *ret;
        return (ret = getenv(get_str(car(args)))) ? 
                      mk_str(l, lstrdup(ret)) : 
                      gsym_nil();
}

static cell *subr_rand(lisp *l, cell *args) { UNUSED(args);
        return mk_int(l, xorshift128plus(l->random_state));
}

static cell *subr_seed(lisp *l, cell *args) {
        l->random_state[0] = get_int(car(args));
        l->random_state[1] = get_int(CADR(args));
        return gsym_tee();
}

static cell* subr_assoc(lisp *l, cell *args) { UNUSED(l);
        return assoc(car(args), CADR(args));
}

static cell *subr_setlocale(lisp *l, cell *args) { 
        char *ret = NULL; 
        switch(get_int(car(args))) {
        case LC_ALL:      case LC_COLLATE: case LC_CTYPE:
        case LC_MONETARY: case LC_NUMERIC: case LC_TIME:
                ret = setlocale(get_int(car(args)), get_str(CADR(args)));
                break;
        default: RECOVER(l, "\"invalid int value\" '%S", args);
        }
        if(!ret) return gsym_nil(); /*failed to set local*/
        return mk_str(l, lstrdup(ret));
}

static cell *subr_typeof(lisp *l, cell *args) { 
        return mk_int(l, car(args)->type);
}

static cell *subr_close(lisp *l, cell *args) { UNUSED(l);
        cell *x;
        x = car(args);
        x->close = 1;
        io_close(get_io(x));
        return x;
}

static cell *subr_timed_eval(lisp *l, cell *args) { 
        clock_t start, end;
        lfloat used;
        cell *x;
        start = clock();
        x = subr_eval(l, args);
        end = clock();
        used = ((lfloat) (end - start)) / CLOCKS_PER_SEC;
        return cons(l, mk_float(l, used), x);
}

static cell *subr_reverse(lisp *l, cell *args) { 
        if(!cklen(args, 1)) goto fail;
        if(gsym_nil() == car(args)) return gsym_nil();
        switch(car(args)->type) {
        case STRING:
                {       
                        char *s = lstrdup(get_str(car(args))), c;
                        size_t i = 0, len;
                        if(!s) HALT(l, "\"%s\"", "out of memory");
                        if(!cklen(args, 0))
                                return mk_str(l, s);
                        len = get_length(car(args)) - 1;
                        do {
                                c = s[i];
                                s[i] = s[len - i];
                                s[len - i] = c;
                        } while(i++ < (len / 2));
                        return mk_str(l, s);
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

static cell *subr_join(lisp *l, cell *args) { 
        char *sep = "", *r, *tmp;
        if(get_length(args) < 2u || !is_asciiz(car(args))) 
                goto fail;
        sep = get_str(car(args));
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
        r = lstrdup(get_str(car(args)));
        for(args = cdr(args); !is_nil(args); args = cdr(args)) {
                if(!is_asciiz(car(args))) {
                        free(r);
                        goto fail;
                }
                tmp = vstrcatsep(sep, r, get_str(car(args)), NULL);
                free(r);
                r = tmp;
        }
        return mk_str(l, r);
fail:   RECOVER(l, "\"expected (str str...) or (string (str ...))\" %S", args);
        return gsym_error();
}

static cell *subr_regexspan(lisp *l, cell *args) { 
        regex_result rr;
        cell *m = gsym_nil();
        rr = regex_match(get_str(car(args)), get_str(CADR(args)));
        if(rr.result <= 0)
                rr.start = rr.end = get_str(CADR(args)) - 1;
        m = (rr.result < 0 ? 
                  gsym_error() : 
                 (rr.result == 0 ? gsym_nil() : gsym_tee()));
        return cons(l, m, 
                cons(l, mk_int(l, rr.start - get_str(CADR(args))),
                cons(l, mk_int(l, rr.end - get_str(CADR(args))), gsym_nil())));
}

static cell *subr_raise(lisp *l, cell *args) { UNUSED(l);
        return raise(get_int(car(args))) ? gsym_nil(): gsym_tee();
}

static cell *subr_split(lisp *l, cell *args) {
        char *pat, *s, *f;
        cell *op = gsym_nil(), *head;
        regex_result rr;
        pat = get_str(car(args));
        if(!(f = s = lstrdup(get_str(CADR(args))))) 
                HALT(l, "\"%s\"", "out of memory");
        head = op = cons(l, gsym_nil(), gsym_nil());
        for(;;) {
                rr = regex_match(pat, s);
                if(!rr.result || rr.end == rr.start) {
                        set_cdr(op, cons(l, mk_str(l, lstrdup(s)), gsym_nil()));
                        break;
                }
                rr.start[0] = '\0';
                set_cdr(op, cons(l, mk_str(l, lstrdup(s)), gsym_nil()));
                op = cdr(op);
                s = rr.end;
        }
        free(f);
        return cdr(head);
}

static cell *subr_substring(lisp *l, cell *args) { 
        intptr_t left, right, tmp;
        char *subs;
        assert(((int)get_length(args)) > 0);
        if(!(get_length(args) == 2 || get_length(args) == 3) 
        || !is_asciiz(car(args)) || !is_int(CADR(args)))
                RECOVER(l, "\"expected (string int int?)\" '%S", args);
        if(get_length(args) == 3 && !is_int(CADDR(args)))
                RECOVER(l, "\"expected (string int int?)\" '%S", args);
        left = get_int(CADR(args));
        if(get_length(args) == 2) {
                if(left >= 0) {
                        left = MIN(left, (int)get_length(car(args)));
                        assert(left < get_length(car(args)));
                        return mk_str(l, lstrdup(get_str(car(args))+left));
                } else if (left < 0) {
                        left = (int)get_length(car(args)) + left;
                        left = MAX(0, left);
                        if(!(subs = calloc(left + 1, 1)))
                                HALT(l, "\"%s\"", "out of memory");
                        assert((get_length(car(args)) - left) > 0);
                        assert((get_str(car(args)) + left) < (get_str(car(args)) + get_length(car(args))));
                        memcpy(subs, 
                            get_str(car(args)) + left, 
                            get_length(car(args)) - left);
                        return mk_str(l, subs);
                }
        }
        if(((right = get_int(CADDR(args))) < 0) || left < 0)
                RECOVER(l, "\"substring lengths must positive\" '%S", args);
        left = MIN(left, (int)get_length(car(args)));
        if((left + right) >= (int)get_length(car(args))) {
                tmp = (right + left) - get_length(car(args));
                right = right - tmp;
                assert((left + right) <= (int)get_length(car(args)));
        }
        if(!(subs = calloc(right + 1, 1)))
                HALT(l, "\"%s\"", "out of memory");
        memcpy(subs, get_str(car(args)) + left, right);
        return mk_str(l, subs);
}

static cell *subr_format(lisp *l, cell *args) { 
        cell *cret;
        io *o = NULL, *t;
        char *fmt, c;
        int ret = 0, pchar;
        if((get_length(args) < 2) || !is_out(car(args)) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected () (io str any...)\" '%S", args);
        o = get_io(car(args));
        args = cdr(args);
        if(!(t = io_sout(calloc(2, 1), 2)))
                HALT(l, "\"%s\"", "out of memory");
        fmt = get_str(car(args));
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
                                           pchar = get_int(car(args));
                                   } else { /*must be a character*/
                                        if(!cklen(car(args), 1))
                                                goto fail;
                                        pchar = get_str(car(args))[0];
                                   }
                                   ret = io_putc(pchar, t);
                                   args = cdr(args);
                                   break;
                        case 's':  if(is_nil(args) || !is_asciiz(car(args)))
                                           goto fail;
                                   ret = io_puts(get_str(car(args)), t);
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
        cret = mk_str(l, t->p.str); /*t->p.str is not freed by io_close*/
        io_close(t);
        return cret;
fail:   free(t->p.str);
        io_close(t);
        RECOVER(l, "\"format error\" %S", args);
        return gsym_error();
}

static cell *subr_tr(lisp *l, cell *args) { 
        /**@todo this function needs a lot of improvement, as does the
         *       functions that implement the translation routines*/
        tr_state st;
        char *mode, *s1, *s2, *tr, *ret; 
        size_t len;
        mode = get_str(car(args));
        s1   = get_str(CADR(args));
        s2   = get_str(CADDR(args));
        tr   = get_str(CADDDR(args));
        len  = get_length(CADDDR(args));
        memset(&st, 0, sizeof(st));
        switch(tr_init(&st, mode, (uint8_t*)s1, (uint8_t*)s2)) {
                case TR_OK:      break;
                case TR_EINVAL:  RECOVER(l, "\"invalid mode\" \"%s\"", mode);
                case TR_DELMODE: RECOVER(l, "\"set 2 not NULL\" '%S", args);
                default:         RECOVER(l, "\"unknown tr error\" '%S", args);
        }
        if(!(ret = calloc(len + 1, 1)))
                HALT(l, "\"%s\"", "out of memory");
        tr_block(&st, (uint8_t*)tr, (uint8_t*)ret, len);  
        return mk_str(l, ret);
}

static cell *subr_define_eval(lisp *l, cell *args) {
        return extend_top(l, car(args), CADR(args));
}

static cell *subr_validate(lisp *l, cell *args) {
        return VALIDATE(l, "validate", get_int(car(args)), get_str(CADR(args)), CADDR(args), 0) ? gsym_tee() : gsym_nil();
}

static cell *subr_proc_code(lisp *l, cell *args) { UNUSED(l);
        return car(get_proc_code(car(args)));
}

static cell *subr_proc_args(lisp *l, cell *args) { UNUSED(l);
        return get_proc_args(car(args));
}

static cell *subr_validation_string(lisp *l, cell *args) {
        char *s;
        if(is_subr(car(args))) s = get_subr_format(car(args));
        else                   s = get_proc_format(car(args));
        return s ? mk_str(l, lstrdup(s)) : gsym_nil();
}

static cell *subr_doc_string(lisp *l, cell *args) {
        char *s;
        if(is_subr(car(args))) s = get_subr_docstring(car(args));
        else                   s = get_proc_docstring(car(args));
        return s ? mk_str(l, lstrdup(s)) : gsym_nil();
}

static cell *subr_top_env(lisp *l, cell *args) { UNUSED(args);
        return l->top_env;
}

static cell *subr_is_closed(lisp *l, cell *args) {  UNUSED(l);
        if(!cklen(args, 1)) RECOVER(l, "\"expected (any) '%S", args);
        return is_closed(car(args)) ? gsym_tee() : gsym_nil();
}

