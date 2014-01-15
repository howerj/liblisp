/** lisp.c */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "regex.h"
#include "sexpr.h"

typedef enum{
  p_nil,

  p_quote,
  p_if,
  p_set,
  p_define,
  p_begin,
  p_lambda,

  p_add,
  p_sub,
  p_mul,
  p_div,
  p_eq,

  p_end
}primitive;

/** core lisp definitions */
/** require; car, cdr, cons, makeobj, find, extend */
expr eval(expr x, expr env);
expr evlis(expr xs, expr env);
expr progn(expr xs, expr env);
expr apply(expr proc, expr vals, expr env);
lisp initlisp(void);
/** require; primitives (+,-,*,/,eq,cdr,cons,car, and the special forms ) */

expr car(expr e){
  return ((expr *)e->buf)[0];
}

expr cdr(expr e){
  return ((expr *)e->buf)[1];
}

expr element(expr e, unsigned int i){
  return ((expr *)e->buf)[i];
}

expr findsym(expr global, char *name){
  unsigned int i = 0;
  expr current = global,  tmp , tmpchld;
  for(i = 0; i < current->len; i++){
    tmp = element(current,i);
    if(S_SYMBOL == tmp->type)
      if(!strcmp(name,tmp->buf))
        return tmp;
    if(S_PRIMITIVE == tmp->type){
      tmpchld = car(tmp);
      if(!strcmp(name,tmpchld->buf))
        return tmp;
    }
  }
  return NULL;
}

/** primitive structure =
 *  S_PRIMITIVE -> S_SYMBOL
 *              -> S_FUNCTION
 */
bool addprim(expr global, char *name, primitive p,io *e){
  expr ex;
  expr chldsym, chldfunc;
  ex        = wcalloc(sizeof(sexpr_t),1,e);
  chldsym   = wcalloc(sizeof(sexpr_t),1,e);
  chldfunc  = wcalloc(sizeof(sexpr_t),1,e);   

  ex->type = S_PRIMITIVE;
  append(ex,chldsym,e);
  append(ex,chldfunc,e);

  chldsym->type = S_SYMBOL;
  chldsym->len  = strlen(name);
  chldsym->buf  = name;

  chldfunc->type = S_FUNCTION;
  chldfunc->len  = (size_t)p;

  append(global,ex,e);
  return true;
}

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

  addprim(global,"quote",   p_quote,&l->e);
  addprim(global,"if",      p_if,&l->e);
  addprim(global,"set!",    p_set,&l->e);
  addprim(global,"define",  p_define,&l->e);
  addprim(global,"begin",   p_begin,&l->e);
  addprim(global,"lamda",   p_lambda,&l->e);
  addprim(global,"add",     p_add,&l->e);
  addprim(global,"sub",     p_sub,&l->e);
  addprim(global,"mul",     p_mul,&l->e);
  addprim(global,"div",     p_div,&l->e);
  addprim(global,"eq",      p_eq,&l->e);
  return l;
}

expr eval(expr x, expr env){
  unsigned int i;
  expr tmp, tmpchld;
  if(!x)
    return NULL;

  switch(x->type){
      case S_NONE:
          break;
      case S_LIST:
        for (i = 0; i < x->len; i++)
          eval(((expr *) x->buf)[i],env);
        break;
      case S_STRING:
          return x;
      case S_SYMBOL:
          if(NULL!=(tmp=findsym(env,x->buf))){
            eval(tmp,env);
          }
          break;
      case S_INTEGER:
          return x;
      case S_PRIMITIVE:
          tmp = car(x);
          printf("#%s\n",(char *)tmp->buf);
          break;
      case S_FILE:
          break;
      case S_POINTER:
          break;
      case S_FUNCTION:
          break;
      default:
          break;
  }
  return NULL;
}

int main(void)
{
  /*
     while (1) {
     print(eval(read(stdin)));
     } */

  lisp l;
  expr x;

  l = initlisp();

  while((x = parse_term(&l->i, &l->e))){
    print_expr(x,&l->o,0,&l->e);
    eval(x,l->global);
    free_expr(x, &l->e);
  }

#if 0
  int ret;
  /** sexpr test*/
  char *in = "((data da\\(\\)ta \"quot\\\\ed data\" 123 4.5)\n"
      " (\"data\" (!@# (4.5) \"(mo\\\"re\" \"data)\")))";

  expr x;

  inputf.type = string_in;
  inputf.ptr.string = in;
  outputf.type = file_out;
  outputf.ptr.file = stdout;
  errorf.type = file_out ;
  errorf.ptr.file = stderr;

  x = parse_term(&inputf,&errorf);

  printf("input is:\n%s\n", in);
  printf("parsed as:\n");
  print_expr(x,&outputf,0,&errorf);
  free_expr(x, &errorf);

  /** regex test*/

#define TEST_REGEX(REG,STR)\
  ret = match(REG,STR);\
  if(0 == ret)\
    printf("nomatch %s:%s\n",REG,STR);\
  else if(1 == ret)\
    printf("match! %s:%s\n",REG,STR);\
  else if(-1 == ret)\
    printf("depth exceeded %s:%s\n",REG,STR);\
  else\
    printf("ERROR %d %s:%s\n",ret,REG,STR);

  TEST_REGEX("hel.o", "hello");
  TEST_REGEX("hel.o", "the sun god, helios");
  TEST_REGEX("^hel.os", "the sun god, helios");
  TEST_REGEX("hel.os$", "the sun god, helios");
#endif
  return 0;
}

