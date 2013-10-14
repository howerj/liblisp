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

typedef union{
        int i;                  
        struct cell *p;     
        struct cell (*f) (struct cell *cell);   
        char *sym;
        char *str;
}cell_u;

/*Our basic lispy data type*/
struct cell {
        cell_e type;
        cell_u car;
        cell_u cdr;
};


typedef struct cell cell_t;

/** cell manipulation macros */
#define car(x)        ((x)->car)        /** car, obviously */
#define cdr(x)        ((x)->cdr)        /** cdr, obviously */
#define settype(x,y)  ((x)->type = (y)) /** set the cell type equal to y*/
#define setcar(x,y)   ((x)->car  = (y))  
#define setcdr(x,y)   ((x)->cdr  = (y))
#define isnil(x)      ((x) == nil)      /** points to null cell? */
#define symname(x)    ((x)->car->sym)   /** symbol name */
#define strname(x)    ((x)->car->str)   /** string name */
#define mksym(x)        /** make a symbol cell */
#define mkstr(x)        /** make a string cell */
#define mkint(x)        /** make an integer cell */
#define mkprim(x)     /** make a primitive operation cell */
#define mkproc(x)     /** make a process cell */

typedef struct{
  FILE *input;
  FILE *output;
  FILE *error;

  cell_t *parse;
  cell_t *state;
  cell_t *print;
} environment_t;


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
