/** @file       valid.c
 *  @brief      validate a list against a type format string, see the
 *              "liblisp.h" header or the code for the format options.
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com 
 *  
 *  @todo print_type_string should decode the format string
 *  @todo print the doc string for the subroutine or procedure if available
 *  @todo __LINE__, __FILE__ and __func__ debugging information should be
 *        passed into lisp_validate with a nice wrapper macro
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

static int print_type_string(lisp *l, unsigned len, char *fmt, cell *args) {
        char c, *s;
        lisp_printf(l, l->efp, 0, "(error \"incorrect arguments\" %d (", (intptr_t)len); 
        while((c = *fmt++)) {
                s = "";
                switch(c) {
                case ' ': continue;
                case 's': s = "symbol";       break; /*symbol*/
                case 'd': s = "integer";      break; /*integer*/
                case 'c': s = "cons";         break; /*cons cell*/
                case 'p': s = "procedure";    break; /*procedure*/
                case 'r': s = "subroutine";   break; /*subroutine*/
                case 'S': s = "string";       break; /*string*/
                case 'P': s = "io-port";      break; /*I/O port*/
                case 'h': s = "hash";         break; /*hash*/
                case 'F': s = "f-expr";       break; /*f-expression*/
                case 'f': s = "float";        break; /*float*/
                case 'u': s = "user-defined"; break; /*user define type*/

                case 'b': s = "t-or-nil";     break; /*nil or tee*/
                case 'i': s = "input-port";   break; /*input port*/
                case 'o': s = "output-port";  break; /*output port*/

                case 'Z': s = "symbol-or-string"; break; /*ASCIIZ (string or symbol)*/
                case 'a': s = "integer-or-float"; break; /*arithmetic type*/
                          /*executable (procedure, subroutine, f-expression)*/
                case 'x': s = "function"; break;
                case 'A': s = "any-expression"; break; /*any*/
                default: HALT(l, "%s", "invalid validation format");
                }
                io_puts(s, l->efp);
                if(*fmt) io_putc(' ', l->efp);
        }
        return lisp_printf(l, l->efp, 0, ") %S)\n", args);
}

int lisp_validate(lisp *l, unsigned len, char *fmt, cell *args, int recover) {
        int v = 1;
        char c, *fmt_head;
        cell *args_head;
        assert(l && fmt && args);
        args_head = args;
        fmt_head = fmt;
        if(!cklen(args, len)) goto fail;
        while((c = *fmt++)) {
                if(is_nil(args) || !v) goto fail;
                v = 0;
                switch(c) {
                case ' ': v = 1; continue;
                case 's': v = is_sym(car(args));      break; /*symbol*/
                case 'd': v = is_int(car(args));      break; /*integer*/
                case 'c': v = is_cons(car(args));     break; /*cons cell*/
                case 'p': v = is_proc(car(args));     break; /*procedure*/
                case 'r': v = is_subr(car(args));     break; /*subroutine*/
                case 'S': v = is_str(car(args));      break; /*string*/
                case 'P': v = is_io(car(args));       break; /*I/O port*/
                case 'h': v = is_hash(car(args));     break; /*hash*/
                case 'F': v = is_fproc(car(args));    break; /*f-expression*/
                case 'f': v = is_floatval(car(args)); break; /*float*/
                case 'u': v = is_userdef(car(args));  break; /*user define type*/

                case 'b': v = is_nil(car(args)) || car(args) == gsym_tee(); break; /*nil or tee*/
                case 'i': v = is_in(car(args));       break; /*input port*/
                case 'o': v = is_out(car(args));      break; /*output port*/

                case 'Z': v = is_asciiz(car(args));   break; /*ASCIIZ (string or symbol)*/
                case 'a': v = is_arith(car(args));    break; /*arithmetic type*/
                          /*executable (procedure, subroutine, f-expression)*/
                case 'x': v = is_subr(car(args)) || is_proc(car(args)) || is_fproc(car(args)); break;
                case 'A': v = 1; break; /*any*/
                default: HALT(l, "%s", "invalid validation format");
                }
                args = cdr(args);
        }
        if(!v) goto fail;
        return 1;
fail:   print_type_string(l, len, fmt_head, args_head);
        if(recover)
                lisp_throw(l, 1);
        return 0;
} 


