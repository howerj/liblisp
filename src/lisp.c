/**
 *  @file           lisp.c
 *  @brief          The Lisp Interpreter
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 *  Experimental, small, lisp interpreter.
 *
 *  Meaning of symbols:
 *  i:      input
 *  o:      output
 *  x:      expression
 *  args:   a list of *lisp_evaluated* arguments
 *  nx:     a newly created expression
 *
 *  At a minimum I should define; quote, atom, eq, car, cdr, cons, cond, define (or
 *  label) and lambda.
 *
 **/

#include "lisp.h"
#include <string.h>
#include <assert.h>
#include "mem.h"
#include "gc.h"
#include "sexpr.h"
#include "regex.h"

/**Avoid warning in primops**/
#define UNUSED(X)  (void)(X)

typedef struct{
        const char *s;
        expr(*func) (expr args, lisp l);
} primop_initializers;

expr mknil(void);

/** 
 * @brief List of primitive operations, used for initialization of structures 
 *        and function declarations. It uses X-Macros to achieve this job.
 *        See  <https://en.wikipedia.org/wiki/X_Macro> and
 *        <http://www.drdobbs.com/cpp/the-x-macro/228700289>
 **/
#define LIST_OF_PRIMITIVE_OPERATIONS\
        PRIMOP_X("+",        primop_add)\
        PRIMOP_X("-",        primop_sub)\
        PRIMOP_X("*",        primop_prod)\
        PRIMOP_X("/",        primop_div)\
        PRIMOP_X("mod",      primop_mod)\
        PRIMOP_X("car",      primop_car)\
        PRIMOP_X("cdr",      primop_cdr)\
        PRIMOP_X("cons",     primop_cons)\
        PRIMOP_X("nth",      primop_nth)\
        PRIMOP_X("length",   primop_len)\
        PRIMOP_X("=",        primop_numeq)\
        PRIMOP_X("print",    primop_printexpr)\
        PRIMOP_X("scar",     primop_scar)\
        PRIMOP_X("scdr",     primop_scdr)\
        PRIMOP_X("scons",    primop_scons)\
        PRIMOP_X("eqt",      primop_typeeq)\
        PRIMOP_X("reverse",  primop_reverse)\
        PRIMOP_X("system",   primop_system)\
        PRIMOP_X("match",    primop_match)
 
/** @brief built in primitives, static declarations **/
#define PRIMOP_X(STRING, FUNCTION) static expr FUNCTION(expr args, lisp l);
LIST_OF_PRIMITIVE_OPERATIONS
#undef PRIMOP_X

/** @brief initializer table for primitive operations **/
#define PRIMOP_X(STRING, FUNCTION) {STRING, FUNCTION},
static primop_initializers primops[] = {
        LIST_OF_PRIMITIVE_OPERATIONS
        {NULL,       NULL} /* this *has* to be the last entry */
};
#undef PRIMOP_X

#define CAR(X)  ((X)->data.cons[0])
#define CDR(X)  ((X)->data.cons[1])

/*** interface functions *****************************************************/

/**
 *  @brief          Initialize the lisp interpreter
 *  @return         A fully initialized lisp environment
 **/
lisp lisp_init(void)
{
        io *e;
        lisp l;
        l = mem_calloc(1, sizeof(*l));
        l->global = mem_calloc(1, sizeof(sexpr_t));
        l->env = mem_calloc(1, sizeof(sexpr_t));

        l->i = mem_calloc(1, io_sizeof_io());
        l->o = mem_calloc(1, io_sizeof_io());

        l->global->type = S_CONS;
        l->env->type = S_CONS;

        /* set up file I/O and pointers */
        io_file_in(l->i, stdin);
        io_file_out(l->o, stdout);

        l->global->data.cons[0] = gc_calloc();
        l->global->data.cons[0]->type = S_NIL;
        l->global->data.cons[1] = NULL;

        e = io_get_error_stream();
        io_file_out(e, stderr); 

        return l;
}

/** 
 *  @brief      Registers a function for use within the lisp environment    
 *  @param      name    functions name
 *  @param      func    function to register.
 *  @param      l       lisp environment to register function in
 *  @return     int     Error code, 0 = Ok, >0 is a failure.
 */
int lisp_register_function(char *name, expr(*func) (expr args, lisp l), lisp l){
        UNUSED(name); UNUSED(func); UNUSED(l);
        return 1;
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
        while (NULL != (x = sexpr_parse(l->i))) {
                x = lisp_eval(x, l->env, l);
                sexpr_print(x, l->o, 0);
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
        fflush(NULL);
        gc_sweep(); /*do not call mark before **this** sweep */ 
        io_file_close(l->o);
        io_file_close(l->i);
        free(l->o);
        free(l->i);
        free(l->global);
        free(l->env);
        mem_free(l);
        return;
}

/**
 *  @brief          Read in an s-expression 
 *  @param          i   Read input from...
 *  @return         A valid s-expression (or NULL), which might be an error!
 **/
expr lisp_read(io * i) { return sexpr_parse(i); }

/**
 *  @brief          Print out an s-expression
 *  @param          x   Expression to print
 *  @param          o   Output stream
 *  @return         void
 **/
void lisp_print(expr x, io * o) { sexpr_print(x, o, 0); }

/**
 *  @brief          Evaluate an already parsed lisp expression
 *  @param          x   The s-expression to parse
 *  @param          env The environment to lisp_evaluate in
 *  @param          l   The global lisp environment
 *  @return         An lisp_evaluated expression, possibly ready for printing.
 **/
expr lisp_eval(expr x, expr env, lisp l)
{
        if(NULL == x){
                SEXPR_PERROR(NULL,"lisp_eval passed NULL");
                exit(EXIT_FAILURE);
        }
        switch(x->type){
        case S_NIL: 
        case S_TEE: 
        case S_INTEGER: 
        case S_STRING:
        case S_PRIMITIVE: 
        case S_PROC: 
        case S_QUOTE:
               return x; 
        case S_CONS: 
               if(S_SYMBOL == CAR(x)->type){
                       return mknil();
               }
               SEXPR_PERROR(x,"Cannot apply");
               return mknil();
        case S_SYMBOL:
               return x;
               break;
        case S_FILE:      IO_REPORT("Not implemented");
        case S_ERROR:     IO_REPORT("Not implemented");
        case S_LAST_TYPE: /*fall through, not a type*/
        default:
                IO_REPORT("Not a valid type");
                exit(EXIT_FAILURE);
        }

        SEXPR_PERROR(NULL,"Should never get here.");
        return x;
}

/**
 *  @brief          Garbage collection
 *  @param          l   Lisp environment to mark and collect
 *  @return         void
 **/
void lisp_clean(lisp l)
{
        gc_mark(l->global);
        gc_sweep();
}

/*** internal helper functions ***********************************************/

expr mknil(void){
        expr x = gc_calloc();
        x->type = S_NIL;
        return x;
}

/*** primitive operations ****************************************************/

/**macro helpers for primops**/
#define INTCHK_R(EXP)\
  if(S_INTEGER!=((EXP)->type)){\
    SEXPR_PERROR((EXP),"arg != integer");\
    return nil;\
  }

/**add a list of numbers**/
static expr primop_add(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**subtract a list of numbers from the 1 st arg**/
static expr primop_sub(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**multiply a list of numbers together**/
static expr primop_prod(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**divide the first argument by a list of numbers**/
static expr primop_div(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**arg_1 modulo arg_2**/
static expr primop_mod(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**car**/
static expr primop_car(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**cdr**/
static expr primop_cdr(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**cons**/
static expr primop_cons(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**NTH element in a list or string**/
static expr primop_nth(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**length of a list or string**/
static expr primop_len(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**test equality of the 1st arg against a list of numbers**/
static expr primop_numeq(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**print**/
static expr primop_printexpr(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**CAR for strings**/
static expr primop_scar(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**cdr for strings**/
static expr primop_scdr(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**cons for strings**/
static expr primop_scons(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**type equality**/
static expr primop_typeeq(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

/**reverse a list or a string**/
static expr primop_reverse(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

static expr primop_system(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

static expr primop_match(expr args, lisp l)
{UNUSED(args); UNUSED(l); return NULL;}

#undef INTCHK_R
#undef UNUSED

/*****************************************************************************/
