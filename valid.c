/** @file       valid.c
 *  @brief      validate a list against a type format string, see the
 *              "liblisp.h" header or the code for the format options.
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com 
 *  
 *  @todo print the doc string for the subroutine or procedure if available
 *  @todo Improve the format options with; variable length validation,
 *        groups of type (such as argument one can be either a string or
 *        and I/O port), optional arguments, etcetera. This will require 
 *        making a strsep() function for "util.c".
 *  @todo add checks for *specific* user defined values, this will
 *        require turning lisp_validate_args into a variadic function, something
 *        like %u will pop an integer off the argument stack to test
 *        against.
 *  @todo make sure the expected argument length format string agree
 *  @todo instead of explicitly passing in the validation string and length
 *        the subroutine or procedure that contains the information could
 *        be passed in.
 **/
 
#include "liblisp.h"
#include "private.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define VALIDATE_XLIST\
        X('s', "symbol",            is_sym(x))\
        X('d', "integer",           is_int(x))\
        X('c', "cons",              is_cons(x))\
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

static int print_type_string(lisp *l, const char *msg, unsigned len, const char *fmt, cell *args)
{
        const char *s, *head = fmt;
        char c;
        io *e = lisp_get_logging(l);
        msg = msg ? msg : "";
        lisp_printf(l, e, 0, 
                "\n(%Berror%t\n %y'validation\n %r\"%s\"\n%t '(%yexpected-length %r%d%t)\n '(%yexpected-arguments%t ", 
                msg, (intptr_t)len); 
        while((c = *fmt++)) {
                s = "";
                switch(c) {
                case ' ': continue;
#define X(CHAR, STRING, ACTION) case (CHAR): s = (STRING); break;
                VALIDATE_XLIST
#undef X
                default: RECOVER(l, "\"invalid format string\" \"%s\" %S))", head, args);
                }
                lisp_printf(l, e, 0, "%y'%s%t", s);
                if(*fmt) io_putc(' ', e);
        }
        return lisp_printf(l, e, 1, ")\n %S)\n", args);
}

size_t validate_arg_count(const char *fmt) {
        size_t i = 0;
        if(!fmt) return 0;
        for(;*fmt; i++) {
                while(*fmt && isspace(*fmt++));
                while(*fmt && !isspace(*fmt++));
        }
        return i;
}

int lisp_validate_cell(lisp *l, cell *x, cell *args, int recover) {
        char *msg, *fmt;
        assert(x && is_func(x));
        msg = is_subr(x) ? get_subr_docstring(x) : get_proc_docstring(x);
        msg = msg ? msg : "";
        fmt = is_subr(x) ? get_subr_format(x) : get_proc_format(x);
        if(!fmt)
                return 1; /*as there is no validation string, its up to the function*/
        return lisp_validate_args(l, msg, get_length(x), fmt, args, recover);
}

int lisp_validate_args(lisp *l, const char *msg, unsigned len, const char *fmt, cell *args, int recover) {
        assert(l && fmt && args && msg);
        int v = 1;
        char c;
        const char *fmt_head;
        cell *args_head, *x;
        assert(l && fmt && args);
        args_head = args;
        fmt_head = fmt;
        if(!cklen(args, len)) goto fail;
        while((c = *fmt++)) {
                if(is_nil(args) || !v || is_closed(car(args))) goto fail;
                v = 0;
                x = car(args);
                switch(c) {
                case ' ': v = 1; continue;
#define X(CHAR, STRING, ACTION) case (CHAR): v = ACTION; break;
                VALIDATE_XLIST
#undef X
                default: RECOVER(l, "\"%s\"", "invalid validation format");
                }
                args = cdr(args);
        }
        if(!v) goto fail;
        return 1;
fail:   print_type_string(l, msg, len, fmt_head, args_head);
        if(recover)
                lisp_throw(l, 1);
        return 0;
} 

