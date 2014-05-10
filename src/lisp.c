/*****************************************************************************\
 *  @file           lisp.c                                                   *
 *  @brief          The Lisp Interpreter                                     *
 *  @author         Richard James Howe.                                      *
 *  @copyright      Copyright 2013 Richard James Howe.                       *
 *  @license        GPL v3.0                                                 *
 *  @email          howe.r.j.89@gmail.com                                    *
\*****************************************************************************/

/** 
 *  Experimental, small, lisp interpreter. 
 *
 *  Meaning of symbols:
 *  ERR HANDLE: A restart function should be implemented, this
 *    error should cause a restart.
 *  i: input
 *  o: output
 *  e: standard err
 *  x: expression
 *  ne: new expression
 *
 *  TODO:
 *    * Better error reporting
 *    * Better error handling; a new primitive type should be made
 *      for it, one that can be caught.
 *    * Make the special forms less special!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "io.h"
#include "mem.h"
#include "sexpr.h"
#include "lisp.h"

/** macro helpers **/
#define car(X)      ((expr)((X)->data.list[0]))
#define cdr(X)      ((expr)((X)->data.list[1]))
#define cddr(X)     ((expr)((X)->data.list[2]))
#define cdddr(X)    ((expr)((X)->data.list[3]))
#define nth(X,Y)    ((expr)((X)->data.list[(Y)]))
#define tstlen(X,Y) ((Y)==(X)->len)

/** global-to-file special objects **/
static expr nil, tee;

/** functions necessary for the intepreter **/
static expr mkobj(sexpr_e type, io *e);
static expr mksym(char *s,io *e);
static expr mkprimop(expr (*func)(expr args, lisp l),io *e);
static expr evlis(expr x,expr env,lisp l);
static expr apply(expr proc, expr args, expr env, lisp l);
static expr find(expr env, expr x, io *e);
static expr extend(expr sym, expr val, lisp l);
static bool primcmp(expr x, char *s, io *e);

/** built in primitives **/
static expr primop_fake(expr args, lisp l); /* dummy for certain special forms */
static expr primop_add(expr args, lisp l);
static expr primop_sub(expr args, lisp l);
static expr primop_prod(expr args, lisp l);
static expr primop_div(expr args, lisp l);
static expr primop_cdr(expr args, lisp l);
static expr primop_car(expr args, lisp l);
static expr primop_cons(expr args, lisp l);


/*** interface functions *****************************************************/

/** returns an initialized lisp environment **/
lisp initlisp(void){ 
  lisp l;
  expr global;
  l      = wcalloc(sizeof (lispenv_t),1,NULL);
  global = wcalloc(sizeof (lispenv_t),1,NULL);
  if(!l||!global)
    exit(-1);

  /** set up file I/O and pointers */
  l->i.type     = file_in;
  l->i.ptr.file = stdin;
  l->o.type     = file_out;
  l->o.ptr.file = stdout;
  l->e.type     = file_out ;
  l->e.ptr.file = stderr;
  l->current    = NULL;
  l->global     = global;

  global->type  = S_LIST;
  nil = mkobj(S_NIL,&l->e);
  tee = mkobj(S_TEE,&l->e);

  /* internal symbols */
  extend(mksym("nil", &l->e),nil,l);
  extend(mksym("t", &l->e),tee,l);

  /* special forms */
  extend(mksym("begin", &l->e),mkprimop(primop_fake,&l->e),l);
  extend(mksym("if",    &l->e),mkprimop(primop_fake,&l->e),l);
  extend(mksym("quote", &l->e),mkprimop(primop_fake,&l->e),l);
  extend(mksym("set",   &l->e),mkprimop(primop_fake,&l->e),l);
  extend(mksym("define",&l->e),mkprimop(primop_fake,&l->e),l);
  extend(mksym("lambda",&l->e),mkprimop(primop_fake,&l->e),l);

  /* normal forms, kind of  */
  extend(mksym("add",&l->e),mkprimop(primop_add,&l->e),l);
  extend(mksym("sub",&l->e),mkprimop(primop_sub,&l->e),l);
  extend(mksym("mul",&l->e),mkprimop(primop_prod,&l->e),l);
  extend(mksym("div",&l->e),mkprimop(primop_div,&l->e),l);
  extend(mksym("car",&l->e),mkprimop(primop_car,&l->e),l);
  extend(mksym("cdr",&l->e),mkprimop(primop_cdr,&l->e),l);
  extend(mksym("cons",&l->e),mkprimop(primop_cons,&l->e),l);

  return l;
}

/** evaluate a lisp expression **/
expr eval(expr x, expr env, lisp l){
  unsigned int i;
  io *e = &l->e;

  if(NULL==x){
    report("passed null!");
    abort();
  }

  switch(x->type){
    case S_LIST: 
      if(tstlen(x,0)) /* () */
        return nil;
      if(S_SYMBOL==car(x)->type){
        if(primcmp(x,"if",e)){ /* (if test conseq alt) */
          if(!tstlen(x,4)){
            report("if: argc != 4");
            return nil;
          }
          if(nil == eval(cdr(x),env,l)){
            return eval(cdddr(x),env,l);
          } else {
            return eval(cddr(x),env,l);
          }
        } else if (primcmp(x,"begin",e)){ /* (begin exp ... ) */
          if(tstlen(x,1)){
            return nil;
          }
          for (i = 1; i < x->len - 1; i++){
            eval(nth(x,i),env,l);
          }
          return eval(nth(x,i),env,l);
        } else if (primcmp(x,"quote",e)){ /* (quote exp) */
          if(!tstlen(x,2)){
            report("quote: argc != 1");/** ERR HANDLE*/
            return nil;
          }
          return cdr(x);
        } else if (primcmp(x,"set",e)){
          expr ne;
          if(!tstlen(x,3)){
            report("set: argc != 2");/** ERR HANDLE*/
            return nil;
          }
          ne = find(l->global,cdr(x),&l->e);
          if(nil == ne){
            return nil;
          }
          ne->data.list[1] = eval(cddr(x),env,l);
          return cdr(ne);
        } else if (primcmp(x,"define",e)){ /*what to do if already defined?*/
          if(!tstlen(x,3)){
            report("define: argc != 2");/** ERR HANDLE*/
            return nil;
          }
          return extend(cdr(x),eval(cddr(x),env,l),l);
        } else if (primcmp(x,"lambda",e)){
        } else {
          return apply(eval(car(x),env,l),evlis(x,env,l),env,l); 
        }
      } else {
        report("cannot apply");
        print_expr(car(x),&l->o,0,e);
      }
      break; 
    case S_SYMBOL:/*if symbol found, return it, else error; unbound symbol*/
      {
        expr ne = find(l->global,x,&l->e);
        if(nil == ne){
          return nil;
        }
        return cdr(find(l->global,x,&l->e));
      }
    case S_FILE: /* to implement */
      report("file type unimplemented");
      return nil; 
    case S_NIL:
    case S_TEE:
    case S_STRING:
    case S_PROC:
    case S_INTEGER:
    case S_PRIMITIVE:
      return x; 
    default:
      report("Serious error, unknown type"); /** ERR HANDLE*/
      abort();
  }

  report("should never get here");
  return x;
}

/*** internal functions ******************************************************/

static expr find(expr env, expr x, io *e){
  unsigned int i;
  char *s = x->data.symbol; 
  for(i = 0; i < env->len; i++){
    if(!strcmp(car(nth(env,i))->data.symbol, s)){
      return nth(env,i);
    }
  }
  report("unbound symbol");
  return nil; 
}

static expr extend(expr sym, expr val, lisp l){
  expr ne = mkobj(S_LIST,&l->e);
  append(ne,sym,&l->e);
  append(ne,val,&l->e);
  append(l->global,ne,&l->e);
  return val;
}

static expr mkobj(sexpr_e type,io *e){
  expr x;
  x = wcalloc(sizeof(sexpr_t), 1,e);
  x->len = 0;
  x->type = type;
  return x;
}

static expr mksym(char *s,io *e){
  expr x;
  x = mkobj(S_SYMBOL,e);
  x->len = strlen(s);
  x->data.symbol = s;
  return x;
}

static expr mkprimop(expr (*func)(expr args, lisp l),io *e){
  expr x;
  x = mkobj(S_PRIMITIVE,e);
  x->data.func = func; 
  return x;
}

static expr primop_fake(expr args, lisp l){
  io *e = &l->e;
  report("This is a place holder, you should never get here");
  print_expr(args,&l->o,0,&l->e);
  return nil;
}

static bool primcmp(expr x, char *s, io *e){
  if(NULL == (car(x)->data.symbol)){
    report("null passed to primcmp!");/** ERR HANDLE*/
    abort();
  }
  return !strcmp(car(x)->data.symbol,s);
}

static expr evlis(expr x,expr env,lisp l){
  unsigned int i;
  expr ne;
  ne = mkobj(S_LIST,&l->e);
  for(i = 1/*skip 0!*/; i < x->len; i++){/** @todo change so it does not use append!*/
    append(ne,eval(nth(x,i),env,l),&l->e);
  }
  return ne;
}

static expr apply(expr proc, expr args, expr env, lisp l){
  io *e = &l->e;
  if(S_PRIMITIVE == proc->type){ 
    return (proc->data.func)(args,l);
  }
  if(S_PROC == proc->type){
  }

  report("Cannot apply expression"); /** ERR HANDLE*/
  return nil;
}


/*** primitive operations ****************************************************/

static expr primop_cons(expr args, lisp l){
  io *e = &l->e;
  expr ne = mkobj(S_LIST,e);
  if(2!=args->len){
    report("cons: argc != 2");
    return nil;
  }
  return ne;
}

static expr primop_car(expr args, lisp l){
  io *e = &l->e;
  expr ne = car(args);
  if(S_LIST != ne->type){
    report("args != list");
    return nil;
  }
  if(1!=args->len){
    report("car: argc != 1");
    return nil;
  }
  return car(ne);
}

static expr primop_cdr(expr args, lisp l){
  io *e = &l->e;
  expr ne = mkobj(S_LIST,e), carg = car(args);
  if((S_LIST != carg->type) || (1>=carg->len)){
    return nil;
  }
  ne->data.list = carg->data.list+1;
  ne->len = carg->len - 1;
  return ne;
}

static expr primop_add(expr args, lisp l){
  io *e = &l->e;
  unsigned int i;
  expr ne;
  ne = mkobj(S_INTEGER,e);
  if(0 == args->len)
    return nil;
  for(i = 0; i < args->len; i++){
    if(S_INTEGER!=nth(args,i)->type){
      report("not an integer type"); /* TODO; print out expr */
      return nil;
    }
    ne->data.integer+=(nth(args,i)->data.integer);
  }
  return ne;
}

static expr primop_prod(expr args, lisp l){
  io *e = &l->e;
  unsigned int i;
  expr ne;
  ne = mkobj(S_INTEGER,e);
  if(0 == args->len)
    return nil;
  ne = nth(args,0);
  for(i = 1; i < args->len; i++){
    if(S_INTEGER!=nth(args,i)->type){
      report("not an integer type"); /* TODO; print out expr */
      return nil;
    }
    ne->data.integer*=(nth(args,i)->data.integer);
  }
  return ne;
}

static expr primop_sub(expr args, lisp l){
  io *e = &l->e;
  unsigned int i;
  expr ne;
  ne = mkobj(S_INTEGER,e);
  if(0 == args->len)
    return nil;
  ne = nth(args,0);
  for(i = 1; i < args->len; i++){
    if(S_INTEGER!=nth(args,i)->type){
      report("not an integer type"); /* TODO; print out expr */
      return nil;
    }
    ne->data.integer-=(nth(args,i)->data.integer);
  }
  return ne;
}

static expr primop_div(expr args, lisp l){
  io *e = &l->e;
  unsigned int i,tmp;
  expr ne;
  ne = mkobj(S_INTEGER,e);
  if(0 == args->len)
    return nil;
  ne = nth(args,0);
  for(i = 1; i < args->len; i++){
    if(S_INTEGER!=nth(args,i)->type){
      report("not an integer type"); /* TODO; print out expr */
      return nil;
    }
    tmp = nth(args,i)->data.integer;
    if(tmp){
      ne->data.integer/=tmp;
    }else{
      report("attempted /0");
      return nil;
    }
  }
  return ne;
}

/*****************************************************************************/
