#ifndef TYPE_H
#define TYPE_H
/** system types */
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>

/*basic types*/
typedef enum{
  false,
  true
} bool; /** be *very* careful with this type*/

typedef uint32_t cell_t; /** standard "machine word" size */

typedef enum { 
  S_NIL,      S_LIST,       S_STRING, S_SYMBOL, 
  S_INTEGER,  S_PRIMITIVE,  S_FILE, S_PROC
} sexpr_e;

/*io module*/
typedef enum { /** enum describing all the io destinations */
  invalid_io,
  file_in,
  file_out,
  string_in,
  string_out
} iotype;

typedef union { /** pointers to where we want to write to or read from */
  FILE *file;
  char *string;
} ioptr;

typedef struct { /** I/O abstraction structure */
  iotype type;            /** what are we abstracting?*/
  ioptr ptr;              /** either FILE* or string */
  unsigned int position;  /** position in string */
  unsigned int max;       /** max string length, if known */
  char c;                 /** character store for wungetc() */
  bool ungetc;            /** true if we have ungetc'ed a character */
} io;

/*sexpr module*/
typedef struct { /** base type for our expressions */
  sexpr_e type;
  size_t len;
  union {
    cell_t integer;
    char *string;
    char *symbol;
    struct sexpr_t **list;
    io *io;
    struct sexpr_t *(*func)(struct sexpr_t **arg, io *e);
  } data;
} sexpr_t, *expr;

#endif
