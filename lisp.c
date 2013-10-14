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
  cons,
  string,
  symbol,
  procedure,
  primitive
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
#define car(x)        ((x)->car)        /** car, obviously */
#define cdr(x)        ((x)->cdr)        /** cdr, obviously */
#define settype(x,y)  ((x)->type = (y)) /** set the cell type equal to y*/
#define setcar(x,y)   ((x)->car  = (y))  
#define setcdr(x,y)   ((x)->cdr  = (y))
#define isnil(x)      ((x) == nil)      /** points to null cell? */
#define mksym(x)        /** make a symbol cell */
#define mkstr(x)        /** make a string cell */
#define mkint(x)        /** make an integer cell */
#define mkprim(x)       /** make a primitive operation cell */
#define mkproc(x,y,x)   /** make a process cell */

typedef struct{
  FILE *input;
  FILE *output;
  FILE *error;

  cell_t *parse;
  cell_t *state;
  cell_t *print;
} environment_t;

cell_t *mkcell(cell_e type, FILE *e, void *p, void *q, void *r){
  cell_t *cell = calloc(1,sizeof(cell_t));
  calloc_fail(e,cell);
  settype(cell,type);
  switch(type){
    case number:    
      car(cell) = p; break;
    case string:
      car(cell) = calloc(strlen(p),sizeof(char));
      calloc_fail(e,cell);
      strcpy(car(cell),p);
    case symbol:
      car(cell) = calloc(strlen(p),sizeof(char));
      calloc_fail(e,cell);
      strcpy(car(cell),p);
    case procedure:
    case primitive:
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
  for(;;){

  }
  return 0;
}
