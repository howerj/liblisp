/** @file       subr.c
 *  @brief      The liblisp interpreter built in subroutines
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *
 *  @todo Subr General to-do; setf, hash-foreach, hash-keys, hash-values, use
 *  arbitrary expressions as keys by serializing them, copy function,
 *  destructive operations (such as +, -, *, ...), use defined operations
 *  for coerce, reverse, copy, arithemtic operations. Longer docstrings. */

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <limits.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
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
	X("car",         subr_car,       "L",    "return the first object in a list")\
	X("cdr",         subr_cdr,       "L",    "return every object apart from the first in a list")\
	X("is-closed",   subr_is_closed, NULL,   "is a object closed?")\
	X("close",       subr_close,     "P",    "close a port, invalidating it")\
	X("coerce",      subr_coerce,    NULL,   "coerce a variable from one type to another")\
	X("cons",        subr_cons,      "A A",  "allocate a new cons cell with two arguments")\
	X("copy",        subr_copy,      "A",    "perform a recursive copy of an expression, if possible")\
	X("define-eval", subr_define_eval, "s A", "extend the top level environment with a computed symbol")\
	X("depth",       subr_depth,     "",      "get the current evaluation depth")\
	X("environment", subr_environment, "",    "get the current environment")\
	X("is-eof",      subr_eofp,      "P",    "is the EOF flag set on a port?")\
	X("eq",          subr_eq,        "A A",  "equality operation")\
	X("eval",        subr_eval,      NULL,   "evaluate an expression")\
	X("ferror",      subr_ferror,    "P",    "is the error flag set on a port")\
	X("flush",       subr_flush,     NULL,   "flush a port")\
	X("foldl",       subr_foldl,    "x c",  "left fold; reduce a list given a function")\
	X("format",      subr_format,    NULL,   "print a string given a format and arguments")\
	X("get-char",    subr_getchar,   "i",    "read in a character from a port")\
	X("get-delim",   subr_getdelim,  "i C",  "read in a string delimited by a character from a port")\
	X("get-system-variable", subr_getenv,    "Z",    "get an environment variable from the system (not thread safe)")\
	X("get-io-str",  subr_get_io_str,"P",    "get a copy of a string from an IO string port")\
	X("hash-create", subr_hash_create,   NULL,   "create a new hash")\
	X("hash-info",   subr_hash_info,     "h",    "get information about a hash")\
	X("hash-insert", subr_hash_insert,   "h Z A", "insert a variable into a hash")\
	X("hash-lookup", subr_hash_lookup,   "h Z",  "loop up a variable in a hash")\
	X("is-input",    subr_inp,       "A",    "is an object an input port?")\
	X("length",      subr_length,    "A",    "return the length of a list or string")\
	X("match",       subr_match,     "Z Z",  "perform a primitive match on a string")\
	X("open",        subr_open,      "d Z",  "open a port (either a file or a string) for reading *or* writing")\
	X("is-output",   subr_outp,      "A",    "is an object an output port?")\
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
	X("set-car",     subr_setcar,    "c A",  "destructively set the first cell of a cons cell")\
	X("set-cdr",     subr_setcdr,    "c A",  "destructively set the second cell of a cons cell")\
	X("signal",      subr_signal,     "d",    "raise a signal")\
	X("&",           subr_band,      "d d",  "bit-wise and of two integers")\
	X("~",           subr_binv,      "d",    "bit-wise inversion of an integers")\
	X("|",           subr_bor,       "d d",  "bit-wise or of two integers")\
	X("^",           subr_bxor,      "d d",  "bit-wise xor of two integers")\
	X("<<",          subr_lshift,    "d d",  "logical left shift an integer")\
	X(">>",          subr_rshift,    "d d",  "logical right shift an integer")\
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
	X("trace",       subr_trace,     "d",    "set the log level, from no errors printed, to copious debugging information")\
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

static lisp_cell_t *forced_add_symbol(lisp_t *l, lisp_cell_t *ob) {
	assert(l && ob);
        assert(hash_lookup(get_hash(l->all_symbols), get_sym(ob)) == NULL);
        if (hash_insert(get_hash(l->all_symbols), get_sym(ob), ob) < 0)
		return NULL;
        return l->tee;
}

lisp_t *lisp_init(void) {
        lisp_t *l;
        io_t *ifp, *ofp, *efp;
        unsigned i;
        if (!(l = calloc(1, sizeof(*l))))  goto fail;
        if (!(ifp = io_fin(stdin)))        goto fail;
        if (!(ofp = io_fout(stdout)))      goto fail;
        if (!(efp = io_fout(stderr)))      goto fail;

	lisp_set_log_level(l, LISP_LOG_LEVEL_ERROR);

        l->gc_off = 1;
        if (!(l->buf = calloc(DEFAULT_LEN, 1))) goto fail;
        l->buf_allocated = DEFAULT_LEN;
        if (!(l->gc_stack = calloc(DEFAULT_LEN, sizeof(*l->gc_stack))))
                goto fail;
        l->gc_stack_allocated = DEFAULT_LEN;

#define X(CNAME, LNAME) l-> CNAME = CNAME;
CELL_XLIST
#undef X
        assert(MAX_RECURSION_DEPTH < INT_MAX);

        /* The lisp init function is now ready to add built in subroutines
         * and other variables, the order in which is does this matters. */
        if (!(l->all_symbols = mk_hash(l, hash_create(DEFAULT_LEN))))
                goto fail;
        if (!(l->top_env = cons(l, cons(l, l->nil, l->nil), l->nil)))
                goto fail;
        if (!(l->top_hash = mk_hash(l, hash_create(DEFAULT_LEN))))
                goto fail;
         set_cdr(l->top_env, cons(l, l->top_hash, cdr(l->top_env)));

        /* Special care has to be taken with the input and output objects
         * as they can change throughout the interpreters lifetime, they
         * should only be set by accessor functions*/
        if (!(l->input   = mk_io(l, ifp))) goto fail;
        if (!(l->output  = mk_io(l, ofp))) goto fail;
        if (!(l->logging = mk_io(l, efp))) goto fail;
        if (!(l->empty_docstr = mk_str(l, lstrdup_or_abort("")))) goto fail;

        l->input->uncollectable = l->output->uncollectable = l->logging->uncollectable = 1;

        /* These are the ports currently used by the interpreter for default
         * input, output and error reporting*/
        if (!lisp_add_cell(l, "*input*",  l->input))   goto fail;
        if (!lisp_add_cell(l, "*output*", l->output))  goto fail;
        if (!lisp_add_cell(l, "*error*",  l->logging)) goto fail;
        /* These are the ports representing stdin/stdout/stderr of the C
         * environment*/
        if (!lisp_add_cell(l, "*stdin*",  mk_io(l, io_fin(stdin))))   goto fail;
        if (!lisp_add_cell(l, "*stdout*", mk_io(l, io_fout(stdout)))) goto fail;
        if (!lisp_add_cell(l, "*stderr*", mk_io(l, io_fout(stderr)))) goto fail;

        for (i = 0; special_cells[i].internal; i++) { /*add special cells*/
                if (!forced_add_symbol(l, special_cells[i].internal))
                        goto fail;
                if (!lisp_extend_top(l, special_cells[i].internal,
                                  special_cells[i].internal))
                        goto fail;
        }

        for (i = 0; integers[i].name; i++) /*add all integers*/
                if (!lisp_add_cell(l, integers[i].name,
                                        mk_int(l, integers[i].val)))
                        goto fail;
	lisp_add_module_subroutines(l, primitives, 0);
        l->gc_off = 0;
        return l;
fail:   l->gc_off = 0;
        lisp_destroy(l);
        return NULL;
}

static lisp_cell_t *subr_band(lisp_t * l, lisp_cell_t * args) {
	return mk_int(l, (uintptr_t)get_int(car(args)) & (uintptr_t)get_int(CADR(args)));
}

static lisp_cell_t *subr_bor(lisp_t * l, lisp_cell_t * args) {
	return mk_int(l, (uintptr_t)get_int(car(args)) | (uintptr_t)get_int(CADR(args)));
}

static lisp_cell_t *subr_bxor(lisp_t * l, lisp_cell_t * args) {
	return mk_int(l, (uintptr_t)get_int(car(args)) ^ (uintptr_t)get_int(CADR(args)));
}

static lisp_cell_t *subr_lshift(lisp_t * l, lisp_cell_t * args) {
	return mk_int(l, (uintptr_t)get_int(car(args)) << (uintptr_t)get_int(CADR(args)));
}

static lisp_cell_t *subr_rshift(lisp_t * l, lisp_cell_t * args) {
	return mk_int(l, (uintptr_t)get_int(car(args)) >> (uintptr_t)get_int(CADR(args)));
}

static lisp_cell_t *subr_binv(lisp_t * l, lisp_cell_t * args) {
	return mk_int(l, ~get_int(car(args)));
}

/** For numerical operations
 * @todo Floating point numbers should infect the operation:
 *       float + integer = float, integer + float = float
 * @todo Add in overloads for user defined types
 * @todo Take an arbitrary number of arguments */

static lisp_cell_t *subr_sum(lisp_t * l, lisp_cell_t * args) {
	lisp_cell_t *x = car(args), *y = CADR(args);
	if (is_int(x))
		return mk_int(l, get_int(x) + get_a2i(y));
	return mk_float(l, get_float(x) + get_a2f(y));
}

static lisp_cell_t *subr_sub(lisp_t * l, lisp_cell_t * args) {
	lisp_cell_t *x = car(args), *y = CADR(args);
	if (is_int(x))
		return mk_int(l, get_int(x) - get_a2i(y));
	return mk_float(l, get_float(x) - get_a2f(y));
}

static lisp_cell_t *subr_prod(lisp_t * l, lisp_cell_t * args) {
	lisp_cell_t *x = car(args), *y = CADR(args);
	if (is_int(x))
		return mk_int(l, get_int(x) * get_a2i(y));
	return mk_float(l, get_float(x) * get_a2f(y));
}

static lisp_cell_t *subr_mod(lisp_t * l, lisp_cell_t * args) {
	intptr_t dividend, divisor;
	dividend = get_int(car(args));
	divisor = get_int(CADR(args));
	if (!divisor || (dividend == INTPTR_MIN && divisor == -1))
		LISP_RECOVER(l, "\"invalid divisor values\"\n '%S", args);
	return mk_int(l, dividend % divisor);
}

static lisp_cell_t *subr_div(lisp_t * l, lisp_cell_t * args) {
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

static lisp_cell_t *subr_greater(lisp_t * l, lisp_cell_t * args) {
	lisp_cell_t *x, *y;
	if (!lisp_check_length(args, 2))
		goto fail;
	x = car(args);
	y = CADR(args);
	if (is_arith(x) && is_arith(y)) {
		return (is_floating(x) ? get_float(x) : get_int(x)) >
		    (is_floating(y) ? get_float(y) : get_int(y)) ? l->tee : l->nil;
	} else if (is_asciiz(x) && is_asciiz(y)) {
		size_t lx = get_length(x), ly = get_length(y);
		if (lx == ly)
			return memcmp(get_str(x), get_str(y), lx) > 0 ? l->tee : l->nil;
		return lx > ly ? l->tee : l->nil;
	}
 fail:	LISP_RECOVER(l, "\"expected (number number) or (string string)\"\n '%S", args);
	return l->error;
}

static lisp_cell_t *subr_less(lisp_t * l, lisp_cell_t * args) {
	lisp_cell_t *x, *y;
	if (!lisp_check_length(args, 2))
		goto fail;
	x = car(args);
	y = CADR(args);
	if (is_arith(x) && is_arith(y)) {
		return (is_floating(x) ? get_float(x) : get_int(x)) <
		    (is_floating(y) ? get_float(y) : get_int(y)) ? l->tee : l->nil;
	} else if (is_asciiz(x) && is_asciiz(y)) {
		size_t lx = get_length(x), ly = get_length(y);
		if (lx == ly)
			return memcmp(get_str(x), get_str(y), lx) < 0 ? l->tee : l->nil;
		return lx < ly ? l->tee : l->nil;
	}
 fail:	LISP_RECOVER(l, "\"expected (number number) or (string string)\"\n '%S", args);
	return l->error;
}

static lisp_cell_t *subr_eq(lisp_t * l, lisp_cell_t * args) {
	/**@warning Most versions of equality treat the floating
	 * point value NaN specially, NaN does not equal NaN,
	 * disregarding the reflexive property that is usually
	 * expected for equality. However this implementation
	 * compares the raw values contained within a cell first,
	 * meaning NaN == NaN, but only on some platforms! (the
	 * size of a lisp float could be greater than or less
	 * than a pointer. What should be done needs to be decided. */
	lisp_cell_t *x, *y;
	x = car(args);
	y = CADR(args);
	if (get_int(x) == get_int(y))
		return l->tee;
	if (is_floating(x) && is_floating(y))
		return get_float(x) == get_float(y) ? l->tee : l->nil;
	if (is_str(x) && is_str(y)) {
		size_t lx = get_length(x), ly = get_length(y);
		if (lx == ly)
			return !memcmp(get_str(x), get_str(y), lx) ? l->tee : l->nil;
	}
	if (is_userdef(x) && is_userdef(y) && get_user_type(x) == get_user_type(y))
		if (l->ufuncs[get_user_type(x)].equal)
			return (l->ufuncs[get_user_type(x)].equal) (x, y) ? l->tee : l->nil;

	return l->nil;
}

static lisp_cell_t *subr_cons(lisp_t * l, lisp_cell_t * args) {
	return cons(l, car(args), CADR(args));
}

static lisp_cell_t *subr_copy(lisp_t * l, lisp_cell_t * args) {
	return lisp_copy(l, car(args));
}

static lisp_cell_t *subr_car(lisp_t * l, lisp_cell_t * args) {
	if (is_nil(car(args)))
		return l->nil;
	return CAAR(args);
}

static lisp_cell_t *subr_cdr(lisp_t * l, lisp_cell_t * args) {
	if (is_nil(car(args)))
		return l->nil;
	return CDAR(args);
}

static lisp_cell_t *subr_setcar(lisp_t * l, lisp_cell_t * args) {
	UNUSED(l);
	set_car(car(args), CADR(args));
	return car(args);
}

static lisp_cell_t *subr_setcdr(lisp_t * l, lisp_cell_t * args) {
	UNUSED(l);
	lisp_cell_t *c = car(args);
	set_cdr(c, CADR(args));
	return car(args);
}

static lisp_cell_t *subr_match(lisp_t * l, lisp_cell_t * args) {
	return match(get_sym(car(args)), get_sym(CADR(args))) ? l->tee : l->nil;
}

static lisp_cell_t *subr_scons(lisp_t * l, lisp_cell_t * args) {
	char *ret;
	ret = CONCATENATE(get_str(car(args)), get_str(CADR(args)));
	return mk_str(l, ret);
}

static lisp_cell_t *subr_scar(lisp_t * l, lisp_cell_t * args) {
	char c[2] = { '\0', '\0' };
	c[0] = get_str(car(args))[0];
	return mk_str(l, lisp_strdup(l, c));
}

static lisp_cell_t *subr_scdr(lisp_t * l, lisp_cell_t * args) {
	if (!(get_str(car(args))[0]))
		mk_str(l, lisp_strdup(l, ""));
	return mk_str(l, lisp_strdup(l, &get_str(car(args))[1]));;
}

static lisp_cell_t *subr_eval(lisp_t * l, lisp_cell_t * args) {
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
		return l->error;
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

static lisp_cell_t *subr_trace(lisp_t * l, lisp_cell_t * args) {
	lisp_log_level level = get_int(car(args));
	switch (level) {
	case LISP_LOG_LEVEL_OFF:
	case LISP_LOG_LEVEL_ERROR:
	case LISP_LOG_LEVEL_NOTE:
	case LISP_LOG_LEVEL_DEBUG:
		lisp_set_log_level(l, level);
		break;
	default:
		LISP_RECOVER(l, "%r\"invalid log level\"\n %m%d%t", (intptr_t)level);
	}
	return l->tee;
}

static lisp_cell_t *subr_length(lisp_t * l, lisp_cell_t * args) {
	return mk_int(l, (intptr_t) get_length(car(args)));
}

static lisp_cell_t *subr_inp(lisp_t * l, lisp_cell_t * args) {
	return is_in(car(args)) ? l->tee : l->nil;
}

static lisp_cell_t *subr_outp(lisp_t * l, lisp_cell_t * args) {
	return is_out(car(args)) ? l->tee : l->nil;
}

static lisp_cell_t *subr_open(lisp_t * l, lisp_cell_t * args) { /**@todo fix validation string for IO_SOUT*/
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
	return ret == NULL ? l->nil : mk_io(l, ret);
}

static lisp_cell_t *subr_get_io_str(lisp_t *l, lisp_cell_t *args) { /**@todo fix for binary data */
	if (!io_is_string(get_io(car(args))))
		LISP_RECOVER(l, "%r\"get string only works on string output IO ports\"%t '%S", args);
	return mk_str(l, lisp_strdup(l, io_get_string(get_io(car(args)))));
}

static lisp_cell_t *subr_getchar(lisp_t * l, lisp_cell_t * args) {
	int i;
	return (i = io_getc(get_io(car(args)))) >= 0 ? mk_int(l, i) : l->nil;
}

static lisp_cell_t *subr_getdelim(lisp_t * l, lisp_cell_t * args) {
	char *s;
	int ch = is_asciiz(CADR(args)) ? get_str(CADR(args))[0] : get_int(CADR(args));
	return (s = io_getdelim(get_io(car(args)), ch)) ? mk_str(l, s) : l->nil;
}

static lisp_cell_t *subr_read(lisp_t * l, lisp_cell_t * args) {
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
		return l->error;
	}
	s = NULL;
	x = NULL;
	io_t *i = NULL;
	if (!(i = is_in(car(args)) ? get_io(car(args)) : io_sin(get_str(car(args)), get_length(car(args)))))
		lisp_out_of_memory(l);
	x = (x = reader(l, i)) ? x : l->error;
	if (s)
		io_close(i);
	LISP_RECOVER_RESTORE(restore_used, l, restore);
	l->errors_halt = errors_halt;
	return x;
}

static lisp_cell_t *subr_puts(lisp_t * l, lisp_cell_t * args) {
	return io_puts(get_str(CADR(args)), get_io(car(args))) < 0 ? l->nil : CADR(args);
}

static lisp_cell_t *subr_putchar(lisp_t * l, lisp_cell_t * args) {
	return io_putc(get_int(CADR(args)), get_io(car(args))) < 0 ? l->nil : CADR(args);
}

static lisp_cell_t *subr_print(lisp_t * l, lisp_cell_t * args) {
	return printer(l, get_io(car(args)), CADR(args), 0) < 0 ? l->nil : CADR(args);
}

static lisp_cell_t *subr_flush(lisp_t * l, lisp_cell_t * args) {
	if (lisp_check_length(args, 0))
		return mk_int(l, fflush(NULL));
	if (lisp_check_length(args, 1) && is_io(car(args)))
		return io_flush(get_io(car(args))) ? l->nil : l->tee;
	LISP_RECOVER(l, "\"expected () or (io)\"\n '%S", args);
	return l->error;
}

static lisp_cell_t *subr_tell(lisp_t * l, lisp_cell_t * args) {
	return mk_int(l, io_tell(get_io(car(args))));
}

static lisp_cell_t *subr_seek(lisp_t * l, lisp_cell_t * args) {
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

static lisp_cell_t *subr_eofp(lisp_t * l, lisp_cell_t * args) {
	return io_eof(get_io(car(args))) ? l->tee : l->nil;
}

static lisp_cell_t *subr_ferror(lisp_t * l, lisp_cell_t * args) {
	return io_error(get_io(car(args))) ? l->tee : l->nil;
}

static lisp_cell_t *subr_remove(lisp_t * l, lisp_cell_t * args) {
	return remove(get_str(car(args))) ? l->nil : l->tee;
}

static lisp_cell_t *subr_rename(lisp_t * l, lisp_cell_t * args) {
	return rename(get_str(car(args)), get_str(CADR(args))) ? l->nil : l->tee;
}

static lisp_cell_t *subr_hash_lookup(lisp_t * l, lisp_cell_t * args) { /*arbitrary expressions could be used as keys if they are serialized to strings first*/
	lisp_cell_t *x;
	return (x = hash_lookup(get_hash(car(args)), get_sym(CADR(args)))) ? x : l->nil;
}

static lisp_cell_t *subr_hash_insert(lisp_t * l, lisp_cell_t * args) {
	if (hash_insert(get_hash(car(args)),
			get_sym(CADR(args)), cons(l, CADR(args), CADR(cdr(args)))))
		lisp_out_of_memory(l);
	return car(args);
}

static lisp_cell_t *subr_hash_create(lisp_t * l, lisp_cell_t * args) {
	hash_table_t *ht = NULL;
	if (get_length(args) % 2)
		goto fail;
	if (!(ht = hash_create(SMALL_DEFAULT_LEN)))
		lisp_out_of_memory(l);
	for (; !is_nil(args); args = cdr(cdr(args))) {
		if (!is_asciiz(car(args)))
			goto fail;
		if (hash_insert(ht, get_sym(car(args)), cons(l, car(args), CADR(args))) < 0)
			lisp_out_of_memory(l);
	}
	return mk_hash(l, ht);
 fail:	hash_destroy(ht);
	ht = NULL;
	LISP_RECOVER(l, "\"expected ({symbol any}*)\"\n '%S", args);
	return l->error;
}

static lisp_cell_t *subr_hash_info(lisp_t * l, lisp_cell_t * args) {
	hash_table_t *ht = get_hash(car(args));
	return mk_list(l,
		       mk_float(l, hash_get_load_factor(ht)),
		       mk_int(l,   hash_get_replacements(ht)),
		       mk_int(l,   hash_get_collision_count(ht)),
		       mk_int(l,   hash_get_number_of_bins(ht)), NULL);
}

static lisp_cell_t *subr_coerce(lisp_t * l, lisp_cell_t * args) {
	if (!lisp_check_length(args, 2) && !is_int(car(args)))
		goto fail;
	return lisp_coerce(l, get_int(car(args)), CADR(args));
 fail:	LISP_RECOVER(l, "\"expected (int any)\"\n %S", args);
	return l->error;
}

lisp_cell_t *lisp_coerce(lisp_t * l, lisp_type type, lisp_cell_t *from) {
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
			head = x = cons(l, l->nil, l->nil);
			if (!fromlen)
				return cons(l, mk_str(l, lstrdup_or_abort("")), l->nil);
			for (i = 0; i < fromlen; i++) {
				/**@todo fix for binary data */
				char c[2] = { '\0', '\0' };
				c[0] = get_str(from)[i];
				y = mk_str(l, lisp_strdup(l, c));
				set_cdr(x, cons(l, y, l->nil));
				x = cdr(x);
			}
			return cdr(head);
		}
		if (is_hash(from)) {	/*hash to list */
			hash_entry_t *cur;
			hash_table_t *h = get_hash(from);
			head = x = cons(l, l->nil, l->nil);
			for (j = 0, i = 0; i < h->len; i++)
				if (h->table[i])
					for (cur = h->table[i]; cur; cur = cur->next, j++) {
						lisp_cell_t *tmp = (lisp_cell_t *) cur->val;
						if (!is_cons(tmp))	/*handle special case for all_symbols hash */
							tmp = cons(l, tmp, tmp);
						set_cdr(x, cons(l, tmp, l->nil));
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
			s = lisp_calloc(l, get_length(x) + 1);
			for (i = 0; !is_nil(x); x = cdr(x), i++)
				s[i] = get_int(car(x));
			return mk_str(l, s);
		}
		break;
	case SYMBOL:
		if (is_str(from))
			if (!strpbrk(get_str(from), " `,!;#()\t\n\r'\"\\"))
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
	return l->error;
}

static lisp_cell_t *subr_assoc(lisp_t * l, lisp_cell_t * args) {
	UNUSED(l);
	return lisp_assoc(car(args), CADR(args));
}

static lisp_cell_t *subr_typeof(lisp_t * l, lisp_cell_t * args) {
	return mk_int(l, car(args)->type);
}

static lisp_cell_t *subr_close(lisp_t * l, lisp_cell_t * args) {
	UNUSED(l);
	lisp_cell_t *x = car(args);
	x->close = 1;
	io_close(get_io(x));
	return x;
}

static lisp_cell_t *subr_reverse(lisp_t * l, lisp_cell_t * args) {
	if (!lisp_check_length(args, 1))
		goto fail;
	if (l->nil == car(args))
		return l->nil;
	switch (car(args)->type) {
	case STRING:
		{
			char *s = lisp_strdup(l, get_str(car(args)));
			size_t len;
			if (lisp_check_length(car(args), 0))
				return mk_str(l, s);
			len = get_length(car(args));
			return mk_str(l, breverse(s, len - 1));
		}
		break;
	case CONS:
		{
			lisp_cell_t *x = car(args), *y = l->nil;
			if (!is_cons(cdr(x)) && !is_nil(cdr(x)))
				return cons(l, cdr(x), car(x));
			for (; is_cons(x); x = cdr(x))
				y = cons(l, car(x), y);
			if (!is_nil(x))
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
			for (i = 0; i < old->len; i++)
				if (old->table[i])
					for (cur = old->table[i]; cur; cur = cur->next) {
						lisp_cell_t *key, *val;
						/**@warning weird hash stuff*/
						if (is_cons(cur->val) && is_asciiz(cdr(cur->val))) {
							key = cdr(cur->val);
							val = car(cur->val);
						} else if (!is_cons(cur->val) && is_asciiz(cur->val)) {
							key = cur->val;
							val = mk_str(l, lisp_strdup(l, cur->key));
						} else {
							goto hfail;
						}
						if (hash_insert(new, get_str(key), cons(l, key, val)) < 0)
							lisp_out_of_memory(l);
					}
			return mk_hash(l, new);
hfail:
			hash_destroy(new);
			new = NULL;
			LISP_RECOVER(l, "\"%s\" '%S", "unreversible hash", car(args));
		}
	default:
		break;
	}
 fail:	LISP_RECOVER(l, "\"expected () (string) (list)\"\n '%S", args);
	return l->error;
}

static lisp_cell_t *subr_signal(lisp_t * l, lisp_cell_t * args) {
	return raise(get_int(car(args))) ? l->nil : l->tee;
}

static lisp_cell_t *subr_substring(lisp_t * l, lisp_cell_t * args) {
	intptr_t left, right, tmp;
	char *subs;
	/**@todo sort this function out*/
	if (!get_length(args))
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
			subs = lisp_calloc(l, left + 1);
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
	subs = lisp_calloc(l, right + 1);
	memcpy(subs, get_str(car(args)) + left, right);
	return mk_str(l, subs);
fail:
	LISP_RECOVER(l, "\"expected (string int int?)\"\n '%S", args);
	return l->error;
}

static lisp_cell_t *subr_format(lisp_t * l, lisp_cell_t * args) {
	/** @note This function has a lot of scope for improvement;
         *        colorization, printing different base numbers, leading
         *        zeros on numbers, printing repeated characters and even
         *        string interpolation ("$x" could look up 'x), as well
         *        as printing out escaped strings.
	 *  @bug  What happens if it cannot write to a file!? */
	lisp_cell_t *cret;
	io_t *o = NULL, *t = NULL;
	char *fmt, c, *ts; /*, *end = NULL;*/
	int ret = 0, pchar, base = 0; /*, precision = 0, precision_set = 0;*/
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
		lisp_out_of_memory(l);
	fmt = get_str(car(args));
	args = cdr(args);
	while ((c = *fmt++))
		if (ret == EOF)
			goto fail;
		else if ('%' == c) {
			switch ((c = *fmt++)) {
			case '\0':
				goto fail;
			/*case '1': case '2': case '3': case '4': case '5':
			case '6': case '7': case '8': case '9':
				errno = 0;
				precision = strtol(fmt--, &end, 10);
				if (errno)
					goto fail;
				fmt = end;
				precision_set = 1;
				break;*/
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
			case 'x': case 'o': case 'u': case 'b':
				switch (c) {
				case 'x': base = 16u; break;
				case 'u': base = 10u; break;
				case 'o': base = 8u; break;
				case 'b': base = 2u;  break;
				default: FATAL("'unreachable");
				}
				if (is_nil(args) || !is_int(car(args)))
					goto fail;
				if (!(ts = utostr(get_int(car(args)), base)))
					lisp_out_of_memory(l);
				switch (c) {
				case 'x': io_puts("0x", t); break;
				case 'u': break;
				case 'o': io_puts("0",  t); break;
				case 'b': io_puts("0b", t); break;
				default: FATAL("'unreachable");
				}
				ret = io_puts(ts, t);
				free(ts);
				args = cdr(args);
				break;
			default:
				goto fail;
			}
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
	return l->error;
}

static lisp_cell_t *subr_tr(lisp_t * l, lisp_cell_t * args) {
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
		break; /* breaks here are to suppress warnings */
	case TR_DELMODE:
		LISP_RECOVER(l, "\"set 2 not NULL\"\n '%S", args);
		break;
	default:
		LISP_RECOVER(l, "\"unknown tr error\"\n '%S", args);
		break;
	}
	ret = lisp_calloc(l, len + 1);
	tr_block(&st, (uint8_t *) tr, (uint8_t *) ret, len);
	return mk_str(l, ret);
}

static lisp_cell_t *subr_define_eval(lisp_t * l, lisp_cell_t * args) {
	return lisp_extend_top(l, car(args), CADR(args));
}

static lisp_cell_t *subr_top_env(lisp_t * l, lisp_cell_t * args) {
	UNUSED(args);
	return l->top_env;
}

static lisp_cell_t *subr_depth(lisp_t * l, lisp_cell_t * args) {
	UNUSED(args);
	return mk_int(l, l->cur_depth);
}

static lisp_cell_t *subr_raw(lisp_t * l, lisp_cell_t * args) {
	return mk_int(l, (intptr_t) get_raw(car(args)));
}

static lisp_cell_t *subr_environment(lisp_t * l, lisp_cell_t * args) {
	UNUSED(args);
	return l->cur_env;
}

static lisp_cell_t *subr_all_syms(lisp_t * l, lisp_cell_t * args) {
	UNUSED(args);
	return l->all_symbols;
}

static lisp_cell_t *subr_getenv(lisp_t * l, lisp_cell_t * args) {
	char *ret;
	return (ret = getenv(get_str(car(args)))) ? mk_str(l, lisp_strdup(l, ret)) : l->nil;
}

static lisp_cell_t *subr_is_closed(lisp_t * l, lisp_cell_t * args) {
	if (!lisp_check_length(args, 1))
		LISP_RECOVER(l, "%r\"expected (any)\"%t\n '%S", args);
	return is_closed(car(args)) ? l->tee : l->nil;
}

static lisp_cell_t *subr_foldl(lisp_t * l, lisp_cell_t * args) {
	lisp_cell_t *f, *start, *tmp, *ret;
	f = car(args);
	tmp = CADR(args);
	start = eval(l, l->cur_depth, car(tmp), l->cur_env);
	tmp = cdr(tmp);
	for (ret = start; is_cons(tmp); tmp = cdr(tmp)) {
		ret = mk_list(l, l->quote, ret, NULL);
		ret = eval(l, l->cur_depth, mk_list(l, f, car(tmp), ret, NULL), l->cur_env);
	}
	if (!is_nil(tmp))
		LISP_RECOVER(l, "%r\"cannot foldl a dotted pair\" '%S", args);
	return ret;
}

static lisp_cell_t *subr_base(lisp_t * l, lisp_cell_t * args) {
	intptr_t base = get_int(CADR(args));
	if (base < 2 || base > 36)
		LISP_RECOVER(l, "%r\"base < 2 || base > 36\"%t\n '%S", args);
	return mk_str(l, dtostr(get_int(car(args)), base));
}

/*@note apply-partially https://www.gnu.org/software/emacs/manual/html_node/elisp/Calling-Functions.html */
static lisp_cell_t *subr_apply(lisp_t * l, lisp_cell_t * args) {
	lisp_cell_t *head = args, *prev = args;
	prev = head = args;
	for (args = cdr(args); is_cons(args); prev = args, args = cdr(args))
		if (is_nil(cdr(args)) && is_cons(car(args)))
			set_cdr(prev, car(args));
	return eval(l, l->cur_depth, head, l->cur_env);
}

