/** @file       print.c
 *  @brief      print out S-expressions
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com 
 *
 * @warning The colorization is not really that portable and relies on the
 *          terminal it is running under being able to cope with ANSI escape
 *          sequences, it can be turned off though. **/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <ctype.h>

static int print_escaped_string(lisp_t * l, io_t * o, unsigned depth, char *s)
{
	char c;
	assert(l && o && s);
	lisp_printf(l, o, depth, "%r\"");
	while ((c = *s++)) {
		switch (c) {
		case '\\':
			lisp_printf(l, o, depth, "%m\\\\%r");
			continue;
		case '\n':
			lisp_printf(l, o, depth, "%m\\n%r");
			continue;
		case '\t':
			lisp_printf(l, o, depth, "%m\\t%r");
			continue;
		case '\r':
			lisp_printf(l, o, depth, "%m\\r%r");
			continue;
		case '"':
			lisp_printf(l, o, depth, "%m\\\"%r");
			continue;
		default:
			break;
		}
		if (!isprint(c)) {
			char num[5] = "\\";
			sprintf(num + 1, "%03o", ((unsigned)c) & 0xFF);
			assert(!num[4]);
			lisp_printf(l, o, depth, "%m%s%r", num);
			continue;
		}
		io_putc(c, o);
	}
	return io_putc('"', o);
}

int lisp_printf(lisp_t *l, io_t *o, unsigned depth, char *fmt, ...) 
{
	int ret;
        va_list ap;
        va_start(ap, fmt);
	ret = lisp_vprintf(l, o, depth, fmt, ap);
	va_end(ap);
	return ret;
}

int lisp_vprintf(lisp_t *l, io_t *o, unsigned depth, char *fmt, va_list ap)
{
        intptr_t d;
        unsigned dep;
        double flt;
        char c, f, *s;
        int ret = 0;
        hash_table_t *ht;
        lisp_cell_t *ob;
        while(*fmt) {
                if(ret == EOF) goto finish;
                if('%' == (f = *fmt++)) {
                        switch (f = *fmt++) {
                        case '\0': goto finish;
                        case '%':  ret = io_putc('%', o); break;
			/*@todo case '.':  // handle ".*" for strings */
		        /*@todo change '*' to something less confusing*/
                        case '*':  f = *fmt++;
                                   if(!f) goto finish;
                                   dep = depth;
                                   while(dep--)
                                           ret = io_putc(f, o); 
                                   break;
                        case 'c':  c = va_arg(ap, int);
                                   ret = io_putc(c, o);
                                   break;
                        case 's':  s = va_arg(ap, char*);
                                   ret = io_puts(s, o);
                                   break;
                        case 'd':  d = va_arg(ap, intptr_t);
                                   ret = io_printd(d, o); 
                                   break;
                        case 'f':  flt = va_arg(ap, double);
                                   ret = io_printflt(flt, o);
                                   break;
                        case 'S':  ob = va_arg(ap, lisp_cell_t *);
                                   ret =  printer(l, o, ob, depth);
                                   break;
                        case 'H':  ht = va_arg(ap, hash_table_t *);
                        { 
                          size_t i;
                          hash_entry_t *cur;
                          lisp_printf(l, o, depth, "{");
                          for(i = 0; i < ht->len; i++)
                            if(ht->table[i])
                              /**@warning messy hash stuff*/
                              for(cur = ht->table[i]; cur; cur = cur->next) {
                                io_putc(' ', o);
                                if(is_cons(cur->val) && is_sym(car(cur->val)))
                                        lisp_printf(l, o, depth, "%S", car(cur->val));
                                else
                                        print_escaped_string(l, o, depth, cur->key);
                                if(is_cons(cur->val)) { 
                                        lisp_printf(l, o, depth, "%t %S", cdr(cur->val));
                                } else {
                                        lisp_printf(l, o, depth, "%t %S", cur->val);
                                }
                              }
                          ret = io_puts(" }", o);
                        }
                        break;
                        default:   if(o->color) {
                                        char *color = "";
                                        switch(f) {
                     /*reset*/          case 't': color = "\x1b[0m";  break;
                     /*bold text*/      case 'B': color = "\x1b[1m";  break;
                     /*reverse video*/  case 'v': color = "\x1b[7m";  break;
                     /*black*/          case 'k': color = "\x1b[30m"; break;
                     /*red*/            case 'r': color = "\x1b[31m"; break;
                     /*green*/          case 'g': color = "\x1b[32m"; break;
                     /*yellow*/         case 'y': color = "\x1b[33m"; break;
                     /*blue*/           case 'b': color = "\x1b[34m"; break;
                     /*magenta*/        case 'm': color = "\x1b[35m"; break;
                     /*cyan*/           case 'a': color = "\x1b[36m"; break;
                     /*white*/          case 'w': color = "\x1b[37m"; break;
                                        default: /*return -1:*/ break;
                                        }
                                        ret = io_puts(color, o);
                                  }
                                 break;
                        }
                } else {
                        ret = io_putc(f, o);
                }
        }
finish: 
        return ret;
}

int printer(lisp_t *l, io_t *o, lisp_cell_t *op, unsigned depth) 
{ 
        lisp_cell_t *tmp;
        if(!op) return EOF;
        if(l && depth > MAX_RECURSION_DEPTH) {
                lisp_printf(l, o, 0, "%r<PRINT-DEPTH-EXCEEDED:%d>%t", (intptr_t) depth);
                return -1;
        }
        switch(op->type) {
        case INTEGER: lisp_printf(l, o, depth, "%m%d", get_int(op));   break; 
        case FLOAT:   lisp_printf(l, o, depth, "%m%f", get_float(op)); break; 
        case CONS:    if(depth && o->pretty) lisp_printf(l, o, depth, "\n%* ");
                      io_putc('(', o);
                      for(;;) {
                              printer(l, o, car(op), depth + 1);
                              if(is_nil(cdr(op))) {
                                      io_putc(')', o);
                                      break;
                              }
                              op = cdr(op);
                              if(!is_cons(op)) {
                                      lisp_printf(l, o, depth, " . %S)", op);
                                      break;
                              }
                              io_putc(' ', o);
                      }
                      break;
        case SYMBOL:  if(is_nil(op)) lisp_printf(l, o, depth, "%rnil");
                      else           lisp_printf(l, o, depth, "%y%s", get_sym(op));
                      break;
        case STRING:  print_escaped_string(l, o, depth, get_str(op));       break;
        case SUBR:    lisp_printf(l, o, depth, "%B<SUBR:%d>", get_int(op)); break;
        case PROC: case FPROC: 
                      lisp_printf(l, o, depth+1, 
                                is_proc(op) ? "(%ylambda%t %S %S " :
                                              "(%yflambda%t %S %S ", 
                                                get_func_docstring(op), get_proc_args(op));
                      for(tmp = get_proc_code(op); !is_nil(tmp); tmp = cdr(tmp)) {
                              printer(l, o, car(tmp), depth+1);
                              if(!is_nil(cdr(tmp)))
                                      io_putc(' ', o);
                      }
                      io_putc(')', o);
                      break;
        case HASH:    lisp_printf(l, o, depth, "%H",             get_hash(op)); break;
        case IO:      lisp_printf(l, o, depth, "%B<IO:%s:%d>",  
                                      op->close? "CLOSED" : 
                                      (is_in(op)? "IN": "OUT"), get_int(op)); break;
        case USERDEF: if(l && l->ufuncs[get_user_type(op)].print)
                              (l->ufuncs[get_user_type(op)].print)(o, depth, op);
                      else lisp_printf(l, o, depth, "<USER:%d:%d>",
                                get_user_type(op), get_int(op)); break;
        case INVALID: 
        default:      FATAL("internal inconsistency");
        }
        return lisp_printf(l, o, depth, "%t") == EOF ? EOF : 0;
}

