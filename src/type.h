/**
 *  @file           type.h
 *  @brief          Types used by all sub-modules of the lisp interpreter
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v3.0
 *  @email          howe.r.j.89@gmail.com
 **/

#ifndef TYPE_H
#define TYPE_H
/*system types*/
#include <stdint.h> /* uint32_t */
#include <stdlib.h> /* exit(), EXIT_FAILURE */
#include <stdio.h>  /* ... lots of things ... */

/*basic types*/
typedef enum{
  false,
  true
} bool; /* be *very* careful with this type*/

typedef int32_t cell_t; /* standard "machine word" size */
typedef struct sexpr_t sexpr_t;
typedef sexpr_t *expr;
typedef struct lispenv_t lispenv_t;
typedef lispenv_t *lisp;

typedef enum {
  S_NIL,      /* () */
  S_TEE,      /* #t */
  S_LIST,     /* list */
  S_STRING,   /* string */
  S_SYMBOL,   /* symbol, positive or negative, input in decimal or octal */
  S_INTEGER,  /* integer */
  S_PRIMITIVE,/* a primitive function */
  S_FILE,     /* for file I/O */
  S_PROC      /* lambda procedure */
} sexpr_e;

/*io module*/
typedef enum { /* enum describing all the I/O destinations */
  invalid_io,  /* error on incorrectly set up I/O*/
  file_in,     
  file_out,
  string_in,   /* read from a string, for things like eval("(+ 2 2)") */
  string_out   /* write to a string, if you want */
} iotype;

typedef union { /* pointers to where we want to write to or read from */
  FILE *file;
  char *string;
} ioptr;

/**I/O abstraction structure**/
typedef struct { 
  iotype type;            /* what are we abstracting?*/
  ioptr ptr;              /* either FILE* or string */
  unsigned int position;  /* position in string */
  unsigned int max;       /* max string length, if known */
  char c;                 /* character store for wungetc() */
  bool ungetc;            /* true if we have ungetc'ed a character */
} io;

/**sexpr module**/
struct sexpr_t { /** base type for our expressions */
  size_t len;
  union {
    cell_t integer; 
    char *string;
    char *symbol;
    struct sexpr_t **list;
    io *io;
    expr (*func)(expr args,lisp l); /*primitive operations*/
  } data;
  sexpr_e type;
  unsigned int gcmark : 1; /**he mark of the garbage collector*/
} ;

/**lisp global environment struct**/
struct lispenv_t{ /** a lisp environment */
  io *i; /* input */
  io *o; /* output */
  io *e; /* stderr */
  expr global; /** 
                 * global list of key-value pairs
                 * ((key_0 val_0) (key_1 val_1) ... (key_n val_n))
                **/
  expr env;
};

#endif
