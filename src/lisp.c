/** 
 *  Richard Howe
 *
 *  License: GPL
 *
 *  Experimental, small, lisp interpreter. 
 *
 *  Meaning of symbols:
 *  ERR HANDLE: A restart function should be implemented, this
 *    error should cause a restart.
 *  i: input
 *  o: output
 *  e: standard err
 *  x: expression
 *
 *  TODO:
 *    Better notation for functions being passed in as arguments, must
 *    be some typedef'in that could help
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

static char *usage = "./lisp -hVi <file>";

/** 
 * version should include md5sum calculated from
 *  c and h files, excluding the file it gets put
 *  into. This will be included here.
 *
 *  Generation would be like this:
 *  md5sum *.c *.h | md5sum | more_processing > version/version.h
 *  in the makefile
 */
static char *version = __DATE__ " : " __TIME__ "\n";
static char *help = "\n\
Lisp Interpreter. Richard James Howe\n\
usage:\n\
  ./lisp -hVi <file>\n\
\n\
  -h      Print this help message and exit.\n\
  -V      Print version number and exit.\n\
  -i      Input file.\n\
  <file>  Iff -i given read from instead of stdin.\n\
";

static int getopt(int argc, char *argv[]);

#define car(X)      ((expr)((X)->data.list[0]))
#define cdr(X)      ((expr)((X)->data.list[1]))
#define cddr(X)     ((expr)((X)->data.list[2]))
#define cdddr(X)    ((expr)((X)->data.list[3]))
#define nth(X,Y)    ((expr)((X)->data.list[(Y)]))
#define tstlen(X,Y) ((Y)==(X)->len)

expr mkobj(sexpr_e type, io *e);
expr mksym(char *s,io *e);
expr mkprimop(expr (*func)(expr args, lisp l),io *e);
expr evlis(expr x,expr env,lisp l);
expr eval(expr x, expr env, lisp l);
expr apply(expr proc, expr args, expr env, lisp l);
expr find(expr global, expr x, io *e);
void extend(expr sym, expr val, lisp l);
lisp initlisp(void);
bool primcmp(expr x, char *s, io *e);

expr primop_fake(expr args, lisp l);

expr primop_add(expr args, lisp l);
expr primop_sub(expr args, lisp l);
expr primop_prod(expr args, lisp l);
expr primop_div(expr args, lisp l);
expr primop_cdr(expr args, lisp l);
expr primop_car(expr args, lisp l);

static expr nil, tee;

int main(int argc, char *argv[]){
  lisp l;
  expr x,env=NULL;

  /** setup environment */
  l = initlisp();

  if(1<argc){
    if(getopt(argc,argv)){
        fprintf(stderr,"%s\n",usage);
        return 1;
    }
  } else {

  }

  while((x = parse_term(&l->i, &l->e))){
    x = eval(x,env,l);
    print_expr(x,&l->o,0,&l->e);
    /** TODO:
     * Garbarge collection; everything not marked in the eval
     * gets collected by free_expr
     */
    /*free_expr(x, &l->e);*/
  }

  print_expr(l->global,&l->o,0,&l->e);
  return 0;
}

/** TODO:
 *    - implement input file option
 *    - --""-- output --""--
 *    - execute on string passed in
 */
static int getopt(int argc, char *argv[]){
  int c;
  if(argc<=1)
    return 0;

  if('-' != *argv[1]++){ /** TODO: Open arg as file */
    return 0;
  }

  while((c = *argv[1]++)){
    switch(c){
      case 'h':
        printf("%s",help);
        return 0;
      case 'V':
        printf("%s",version);
        return 0;
      case 'i':
        break;
      default:
        fprintf(stderr,"unknown option: '%c'\n", c);
        return 1;
    }
  }

  return 0;
}

lisp initlisp(void){ /** initializes the environment, nothing special here */
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

  return l;
}


expr find(expr global, expr x, io *e){
  unsigned int i;
  char *s = x->data.symbol; /* programmers job to make sure this is not null!*/
  for(i = 0; i < global->len; i+=2){
    if(!strcmp(nth(global,i)->data.symbol, s)){
      return nth(global,i+1);
    }
  }
  report("unbound symbol");
  return nil; 
}

void extend(expr sym, expr val, lisp l){
  /** TODO: Error handling, sort on list after appending */
  append(l->global,sym,&l->e);
  append(l->global,val,&l->e);
  /** SORT LIST*/
}

expr mkobj(sexpr_e type,io *e){
  expr x;
  x = wcalloc(sizeof(sexpr_t), 1,e);
  x->len = 0;
  x->type = type;
  return x;
}

expr mksym(char *s,io *e){
  expr x;
  x = mkobj(S_SYMBOL,e);
  x->len = strlen(s);
  x->data.symbol = s;
  return x;
}

expr mkprimop(expr (*func)(expr args, lisp l),io *e){
  expr x;
  x = mkobj(S_PRIMITIVE,e);
  x->data.func = func; 
  return x;
}

expr primop_fake(expr args, lisp l){
  io *e = &l->e;
  report("This is a place holder, you should never get here");
  print_expr(args,&l->o,0,&l->e);
  return nil;
}

/*****************************************************************************/
expr primop_numeq(expr args, lisp l){
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
    ne->data.integer=(nth(args,i)->data.integer);
  }
  return ne;

}

expr primop_car(expr args, lisp l){
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

expr primop_cdr(expr args, lisp l){
  io *e = &l->e;
  expr ne = mkobj(S_LIST,e), carg = car(args);
  if((S_LIST != carg->type) || (1>=carg->len)){
    return nil;
  }
  ne->data.list = wmalloc(sizeof(expr) * carg->len,e);
  memcpy(ne->data.list, carg->data.list + 1, sizeof(expr) * (carg->len - 1));
  ne->len = carg->len - 1;
  return ne;
}

expr primop_add(expr args, lisp l){
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

expr primop_prod(expr args, lisp l){
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

expr primop_sub(expr args, lisp l){
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

expr primop_div(expr args, lisp l){
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

bool primcmp(expr x, char *s, io *e){
  if(NULL == (car(x)->data.symbol)){
    report("null passed to primcmp!");/** ERR HANDLE*/
    abort();
  }
  return !strcmp(car(x)->data.symbol,s);
}

expr evlis(expr x,expr env,lisp l){
  unsigned int i;
  expr ne;
  ne = mkobj(S_LIST,&l->e);
  for(i = 1/*skip 0!*/; i < x->len; i++){/** TODO: change so it does not use append!*/
    append(ne,eval(nth(x,i),env,l),&l->e);
  }
  return ne;
}


expr eval(expr x, expr env, lisp l){
  unsigned int i;
  io *e = &l->e;

  if(NULL==x){
    report("passed null!");
    abort();
  }

  switch(x->type){
    case S_LIST: /** most of eval goes here! */
      if(tstlen(x,0)) /* () */
        return nil;
      if(S_SYMBOL==car(x)->type){
        /** TODO: begin, quote, etc, need binding!*/
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
            eval((expr)(x->data.list[i]),env,l);
          }
          return eval((expr)(x->data.list[i]),env,l);
        } else if (primcmp(x,"quote",e)){ /* (quote exp) */
          if(!tstlen(x,2)){
            report("quote: argc != 1");/** ERR HANDLE*/
            return nil;
          }
          return cdr(x);
        } else if (primcmp(x,"set",e)){
          if(!tstlen(x,3)){
            report("set:  argc != 2");/** ERR HANDLE*/
            return nil;
          }
        } else if (primcmp(x,"define",e)){

        } else if (primcmp(x,"lambda",e)){
        } else {
          return apply(eval(car(x),env,l),evlis(x,env,l),env,l); 
        }
      } else {
        /*ne = eval(x,env,l);
        if(S_SYMBOL!=ne->type)*/
          report("cannot apply");/** ERR HANDLE*/
        print_expr(car(x),&l->o,0,e);
      }
      break; 
    case S_SYMBOL:/*if symbol found, return it, else error; unbound symbol*/
      return find(l->global,x,&l->e);
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
expr apply(expr proc, expr args, expr env, lisp l){
  io *e = &l->e;
  if(S_PRIMITIVE == proc->type){ 
    return (proc->data.func)(args,l);
  }
  if(S_PROC == proc->type){
  }

  report("Cannot apply expression"); /** ERR HANDLE*/
  return nil;
}
