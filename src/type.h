/**
 *  @file           type.h
 *  @brief          Types used by all sub-modules and some c lib includes.
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 **/

#ifndef TYPE_H
#define TYPE_H
/*system types*/
#include <stdint.h>             /* intX_t */
#include <stdlib.h>             /* exit(), EXIT_FAILURE */
#include <stdio.h>              /* ... lots of things ... */

/*basic types*/
typedef enum {
        false,
        true
} bool;                         /* be *very* careful with this type */

typedef int32_t cell_t;         /* standard "machine word" size */
typedef struct sexpr_t sexpr_t;
typedef sexpr_t *expr;
typedef struct lispenv_t lispenv_t;
typedef lispenv_t *lisp;

typedef enum {
        S_NIL,                  /* 0:  () */
        S_TEE,                  /* 1:  t */
        S_LIST,                 /* 2:  list */
        S_STRING,               /* 3:  string */
        S_SYMBOL,               /* 4:  symbol */
        S_INTEGER,              /* 5:  integer */
        S_PRIMITIVE,            /* 6:  a primitive function */
        S_FILE,                 /* 7:  for file I/O */
        S_PROC,                 /* 8:  lambda procedure */
        S_QUOTE,                /* 9:  quoted expression */
        S_ERROR                 /* 10: error return and handling */
} sexpr_e;

/**I/O abstraction structure**/
typedef struct {
        enum iotype {
                invalid_io, /* error on incorrectly set up I/O */
                file_in,
                file_out,
                string_in, /* read from a string */
                string_out /* write to a string, if you want */
        }type;

        union {
                FILE *file;
                char *string;
        } ptr; 

        size_t position;        /* position in string */
        size_t max;             /* max string length, if known */
        char c;                 /* character store for io_ungetc() */
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
                 expr(*func) (expr args, lisp l);       /*primitive operations */
        } data;
        sexpr_e type;
        unsigned int mem_gc_mark:1;  /*the mark of the garbage collector */
};

/**lisp global environment struct**/
struct lispenv_t {/** a lisp environment */
        io *i;                  /* input */
        io *o;                  /* output */
        io *e;                  /* stderr */
        expr global; /*global key-val list*/
        expr env;
};

#endif
