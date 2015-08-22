#include "liblisp.h"
#include "private.h"
#include <stdarg.h>

static int print_escaped_string(lisp *l, io *o, unsigned depth, char *s) {
        char c;
        printerf(l, o, depth, "%r\"");
        while((c = *s++)) {
               switch(c) {
               case '\\': printerf(l, o, depth, "%m\\\\%r"); continue;
               case '\n': printerf(l, o, depth, "%m\\n%r");  continue;
               case '\t': printerf(l, o, depth, "%m\\t%r");  continue;
               case '\r': printerf(l, o, depth, "%m\\r%r");  continue;
               case '"':  printerf(l, o, depth, "%m\\\"%r"); continue;
               default: break;
               }
               io_putc(c, o);
        }
        return io_putc('"', o);
}

int printerf(lisp*l, io *o, unsigned depth, char *fmt, ...) {
        va_list ap;
        intptr_t d;
        unsigned dep;
        double flt;
        char c, f, *s;
        int ret = 0;
        hashtable *ht;
        cell *ob;
        va_start(ap, fmt);
        while(*fmt) {
                if(ret == EOF) goto finish;
                if('%' == (f = *fmt++)) {
                        switch (f = *fmt++) {
                        case '\0': goto finish;
                        case '%':  ret = io_putc('%', o); break;
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
                        case 'S':  ob = va_arg(ap, cell*);
                                   ret =  printer(l, o, ob, depth);
                                   break;
                        case 'H':  ht = va_arg(ap, hashtable*);
                        { 
                          size_t i;
                          hashentry *cur;
                          printerf(l, o, depth, "(%yhash-create%t");
                          for(i = 0; i < ht->len; i++)
                            if(ht->table[i])
                              for(cur = ht->table[i]; cur; cur = cur->next) {
                                io_putc(' ', o);
                                print_escaped_string(l, o, depth, cur->key);
                                if(iscons(cur->val)) /**@warning messy hash stuff*/
                                        printerf(l, o, depth, "%t '%S", cdr(cur->val));
                                else    printerf(l, o, depth, "%t '%S", cur->val);
                              }
                          ret = io_putc(')', o);
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
finish: va_end(ap);
        return ret;
}

int printer(lisp *l, io *o, cell *op, unsigned depth) { /*write out s-expr*/
        if(!op) return EOF;
        if(l && depth > l->max_depth) { /*problem if depth UINT_MAX < l->max_depth INTPTR_MAX*/
                printerf(l, o, depth, "%r<PRINT-DEPTH-EXCEEDED:%d>%t", (intptr_t) depth);
                return -1;
        }
        switch(op->type) {
        case INTEGER: printerf(l, o, depth, "%m%d", intval(op));   break; 
        case FLOAT:   printerf(l, o, depth, "%m%f", floatval(op)); break; 
        case CONS:    if(depth && o->pretty) io_putc('\n', o);
                      if(o->pretty) printerf(l, o, depth, "%* ");
                      io_putc('(', o);
                      for(;;) {
                              printer(l, o, car(op), depth + 1);
                              if(isnil(cdr(op))) {
                                      io_putc(')', o);
                                      break;
                              }
                              op = cdr(op);
                              if(op->type != CONS) {
                                      printerf(l, o, depth, " . %S)", op);
                                      break;
                              }
                              io_putc(' ', o);
                      }
                      break;
        case SYMBOL:  if(isnil(op)) printerf(l, o, depth, "%r()");
                      else          printerf(l, o, depth, "%y%s", symval(op));
                      break;
        case STRING:  print_escaped_string(l, o, depth, strval(op));     break;
        case SUBR:    printerf(l, o, depth, "%B<SUBR:%d>", intval(op));  break;
        case PROC:    printerf(l, o, depth+1, "(%ylambda%t %S %S)", 
                                      procargs(op), car(proccode(op)));
                      break;
        case FPROC:   printerf(l, o, depth+1, "(%yflambda%t %S %S)", 
                                      procargs(op), car(proccode(op)));
                      break;
        case HASH:    printerf(l, o, depth, "%H",             hashval(op)); break;
        case IO:      printerf(l, o, depth, "%B<IO:%s:%d>",  
                                      op->close? "CLOSED" : 
                                      (isin(op)? "IN": "OUT"), intval(op)); break;
        case USERDEF: if(l && l->ufuncs[op->userdef].print)
                              (l->ufuncs[op->userdef].print)(o, depth, op);
                      else printerf(l, o, depth, "<USER:%d:%d>",
                                (intptr_t)op->userdef, intval(op)); break;
        case INVALID: 
        default:      FATAL("internal inconsistency");
        }
        return printerf(l, o, depth, "%t") == EOF ? EOF : 0;
}

