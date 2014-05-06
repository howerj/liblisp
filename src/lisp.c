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
expr mkprimop(expr (*func)(expr x, io *e),io *e);
expr eval(expr x, expr env, lisp l);
expr apply(expr x, expr env, lisp l);
expr find(expr global, expr x, io *e);
void extend(expr sym, expr val, lisp l);
lisp initlisp(void);
bool primcmp(expr x, char *s, io *e);

expr primop_add(expr x, io *e);

static expr nil;

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
    /*printf("#%p\n",(void*)eval(x,env,l));*/
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

  extend(mksym("add",&l->e),mkprimop(primop_add,&l->e),l);
  extend(mksym("sub",&l->e),mkprimop(primop_add,&l->e),l);
  extend(mksym("div",&l->e),mkprimop(primop_add,&l->e),l);
  extend(mksym("mul",&l->e),mkprimop(primop_add,&l->e),l);

  return l;
}


expr find(expr global, expr x, io *e){
  unsigned int i;
  char *s = x->data.symbol; /** programmers job to make sure this is not null!*/
  for(i = 0; i < global->len; i+=2){
    if(!strcmp(nth(global,i)->data.symbol, s)){
      return nth(global,i+1);
    }
  }
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

expr mkprimop(expr (*func)(expr x, io *e),io *e){
  expr x;
  x = mkobj(S_PRIMITIVE,e);
  x->data.func = func; /** TODO: check this*/
  return x;
}

expr primop_add(expr x, io *e){
  unsigned int i;
  cell_t sum = 0;
  expr ne;
  ne = mkobj(S_INTEGER,e);
  if(1 >= x->len)
    return nil;
  for(i=1 /*skip add*/; i < x->len; i++){
    /*ne=eval(x,NULL,e);*/
    ne = nth(x,i);
    if(S_INTEGER!=ne->type){
      report("not an integer type");
      return nil;
    }
    sum+=ne->data.integer;
  }
  ne->data.integer = sum;
  return ne;
}

bool primcmp(expr x, char *s, io *e){
  if(NULL == (car(x)->data.symbol)){
    report("null passed to primcmp!");/** ERR HANDLE*/
    abort();
  }
  return !strcmp(car(x)->data.symbol,s);
}

expr eval(expr x, expr env, lisp l){
  unsigned int i;
  io *e = &l->e;
  expr ne;

  if(NULL==x){
    report("passed null!");
  }

  switch(x->type){
    case S_LIST: /** most of eval goes here! */
      /** what should ((quote +) 2 3) do ?*/
      if(tstlen(x,0)) /* () */
        return nil;
      if(S_SYMBOL==car(x)->type){
        if(primcmp(x,"if",e)){ /* (if test conseq alt) */
          if(!tstlen(x,4)){
            report("special form 'if', expected list of size 4");
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
            report("special form 'quote', expected list of size 2");/** ERR HANDLE*/
            return nil;
          }
          return cdr(x);
        } else if (primcmp(x,"set",e)){
          if(!tstlen(x,3)){
            report("special form 'set', expected list of size 3");/** ERR HANDLE*/
            return nil;
          }
        } else if (primcmp(x,"define",e)){
        } else if (primcmp(x,"lambda",e)){
        } else {
          /** symbol look up and apply */
#if 0
          ne = eval(car(x),env,l);
          if((S_PRIMITIVE != ne->type) && (S_PROC != ne->type)){
            report("cannot apply, not a primitive or procedure");/** ERR HANDLE*/
            print_expr(ne,&l->o,0,e);
            return nil;
          }
#endif
          return apply(x,env,l);
        }
      } else {
        /*ne = eval(x,env,l);
        if(S_SYMBOL!=ne->type)*/
          report("cannot apply");/** ERR HANDLE*/
        print_expr(car(x),&l->o,0,e);
      }
      break; 
    case S_SYMBOL:/*if symbol found, return it, else error; unbound symbol*/
    { 
      expr fx = find(l->global,x,&l->e);
      if(nil == fx)
        report("unbound symbol");
      return fx;
    }
    case S_FILE: /* to implement */
      report("file type unimplemented");
      return nil; 
    case S_NIL:
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

expr apply(expr x, expr env, lisp l){
  io *e = &l->e;
  expr ne;
  ne = eval(car(x),env,l);
  if(S_PRIMITIVE == ne->type){
    return (ne->data.func)((struct sexpr_t *)x,e);
    return nil;
  }
  if(S_PROC == ne->type){
  }

  report("Cannot apply expression"); /** ERR HANDLE*/
  return x;
}
