#include "liblisp.h"
#include "private.h"
#include <assert.h>

static cell *compile_inner(lisp *l, unsigned depth, cell *exp, cell *env) { assert(l);
        if(depth > MAX_RECURSION_DEPTH)
                RECOVER(l, "'recursion-depth-reached %d", depth);
        if(!exp || !env) return NULL;
        if(l->trace_on) lisp_printf(l, lisp_get_logging(l), 1, "(%ytrace%t %S)\n", exp); 
        if(l->sig) {
                l->sig = 0;
                lisp_throw(l, 1);
        }
        switch(exp->type) {
        case INTEGER: case SUBR:  case STRING:  case IO: case HASH:
        case FPROC:   case FLOAT: case USERDEF:
                return exp;

        case PROC:    break;

        case SYMBOL:  break;
        case CONS:    break;

        case INVALID:
        default:  HALT(l, "%s", "internal inconsistency: unknown type");
        }
        return exp;
}

cell *compile_expression(lisp *l, unsigned depth, cell *exp, cell *env) { assert(l);
        cell *ret;

        /**/
        ret = compile_inner(l, depth+1, cdr(exp), env);

        return mk_proc(l, car(exp), ret, env);
}

