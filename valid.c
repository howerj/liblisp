/** @file       valid.c
 *  @brief      VALIDATE a list against a type format string, see the
 *              "liblisp.h" header or the code for the format options.
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com 
 *  
 *  @todo print_type_string should decode the format string
 *  @todo print the doc string for the subroutine or procedure if available
 *  @todo Improve the format options with; variable length validation,
 *        groups of type (such as argument one can be either a string or
 *        and I/O port), optional arguments, etcetera. This will require 
 *        making a strsep() function for "util.c".
 *  @todo make sure the expected argument length format string agree
 **/
 
#include "liblisp.h"
#include "private.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <assert.h>

#define VALIDATE_XLIST\
        X('s', "symbol",        /* symbol */              is_sym(x))\
        X('d', "integer",       /* integer */             is_int(x))\
        X('c', "cons",          /* cons cell (list) */    is_cons(x))\
        X('p', "procedure",     /* procedure */           is_proc(x))\
        X('r', "subroutine",    /* built in subroutine */ is_subr(x))\
        X('S', "string",        /* string */              is_str(x))\
        X('P', "io-port",       /* IO port */             is_io(x))\
        X('h', "hash",          /* a hash */              is_hash(x))\
        X('F', "f-expr",        /* an F-Expression */     is_fproc(x))\
        X('f', "float",         /* a floating point number */ is_floatval(x))\
        X('u', "user-defined",  /* user define type */    is_userdef(x))\
        X('b', "t-or-nil",      /* "boolean" */           is_nil(x) || x == gsym_tee())\
        X('i', "input-port",    /* input port only */     is_in(x))\
        X('o', "output-port",   /* output port only */    is_out(x))\
        X('Z', "symbol-or-string", /* group symbol/string */ is_asciiz(x))\
        X('a', "integer-or-float", /* group integer/float */ is_arith(x))\
        X('x', "function",      /* executable type */     is_func(x))\
        X('A', "any-expression",/* any expression */      1)

static int print_type_string(lisp *l, unsigned len, const char *fmt, cell *args, 
                              const char *file, const char *func, unsigned line) 
{
        const char *s, *head = fmt;
        char c;
        io *e = lisp_get_logging(l);
        lisp_printf(l, e, 0, "(error \"incorrect arguments\" (%s %s %d) (%d) (", 
                        file, func, (intptr_t) line, (intptr_t)len); 
        while((c = *fmt++)) {
                s = "";
                switch(c) {
                case ' ': continue;
#define X(CHAR, STRING, ACTION) case (CHAR): s = (STRING); break;
                VALIDATE_XLIST
#undef X
                default: HALT(l, "\"invalid format string\" \"%s\" %S))", head, args);
                }
                io_puts(s, e);
                if(*fmt) io_putc(' ', e);
        }
        return lisp_printf(l, e, 0, ") %S)\n", args);
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

int lisp_validate(lisp *l, unsigned len, char *fmt, cell *args, int recover, 
                           const char *file, const char *func, unsigned line) 
{       assert(l && fmt && args && file && func);
        int v = 1;
        char c, *fmt_head;
        cell *args_head, *x;
        assert(l && fmt && args);
        args_head = args;
        fmt_head = fmt;
        if(!cklen(args, len)) goto fail;
        while((c = *fmt++)) {
                if(is_nil(args) || !v || car(args)->close) goto fail;
                v = 0;
                x = car(args);
                switch(c) {
                case ' ': v = 1; continue;
#define X(CHAR, STRING, ACTION) case (CHAR): v = ACTION; break;
                VALIDATE_XLIST
#undef X
                default: HALT(l, "%s", "invalid validation format");
                }
                args = cdr(args);
        }
        if(!v) goto fail;
        return 1;
fail:   print_type_string(l, len, fmt_head, args_head, file, func, line);
        if(recover)
                lisp_throw(l, 1);
        return 0;
} 


