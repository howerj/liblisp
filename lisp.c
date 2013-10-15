/** Howe Lisp.
 * 
 * @file lisp.c 
 * @brief Small ANSI C Lisp Interpreter
 *
 * @author         Richard James Howe.
 * @copyright      Copyright 2013 Richard James Howe.
 * @license        LGPL      
 * @email          howe.r.j.89@gmail.com
 *
 * Plan
 *  Three syntax trees; 
 *    read -> generates tree
 *    eval -> takes read's tree, interacts with state tree, outputs print tree
 *    print -> takes evals' tree.
 *    loop -> goto read!
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define max_buf_m       (256u)
#define error_m(stream,string) do { \
  fprintf(stream, "%s:%d:%s", string,__LINE__,__FILE__);\
  exit(1);\
  } while (0)
#define calloc_fail(stream,x)  if(NULL == (x)){ error_m(stream, "calloc fail");}

typedef enum{
  number,
  con,
  string,
  symbol,
  primitive,
  procedure
} cell_e;

struct cell;

/*Our basic lispy data type*/
struct cell {
        cell_e type;
        void *car;
        void *cdr;
};


typedef struct cell cell_t;

/** cell manipulation macros */
#define cons(x,y)     mkcell(con, e, (void *) x, (void *) y, NULL)
#define car(x)        ((x)->car)        /** car, obviously */
#define cdr(x)        ((x)->cdr)        /** cdr, obviously */
#define settype(x,y)  ((x)->type = (y)) /** set the cell type equal to y*/
#define setcar(x,y)   ((x)->car  = (y))  
#define setcdr(x,y)   ((x)->cdr  = (y))
#define isnil(x)      ((x) == nil)      /** points to null cell? */
#define mksym(x)      mkcell(symbol, e,(void *) (x), NULL,NULL)
#define mkstr(x)      mkcell(string, e,(void *) (x), NULL,NULL) 
#define mkint(x)      mkcell(number, e,(void *) (x), NULL,NULL) 
#define mkprim(x)     mkcell(primitive, e,(void *) (x), NULL,NULL) 
#define mkproc(x,y,z)   /** make a process cell */

typedef struct{
  FILE *input;
  FILE *output;
  FILE *error;

  cell_t *parse;
  cell_t *state;
  cell_t *print;
} environment_t;

cell_t *mkcell(cell_e type, FILE *e, void *p, void *q, void *r){
  int len;
  cell_t *cell = calloc(1,sizeof(cell_t));
  calloc_fail(e,cell);
  settype(cell,type);
  switch(type){
    case number:    
      car(cell) = p; 
      break;
    case con:
      car(cell) = p;
      cdr(cell) = q;
      break;
    case string: /** fall through */
    case symbol:
      len = strlen(p) + 1;
      car(cell) = (void *)len;
      cdr(cell) = calloc(len,sizeof(char));
      calloc_fail(e,cell);
      strcpy(car(cell),p);
      break;
    case procedure:
    case primitive:
      car(cell) = p; break;
    default:
      error_m(e,"not implemented");
      exit(1);
  }
  return cell;
}

/*cell_t *parse(FILE *input, FILE *error){
  int c;
  while(fgetc(c,input)
  return NULL;
}*/

int main(void){
  environment_t env; 
  env.input  = stdin;
  env.output = stdout;
  env.error  = stderr;
  env.parse  = NULL;
  env.state  = NULL;
  env.print  = NULL;

  return 0;
}
