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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h> 
#include <stdbool.h>            
#include "io.h"

typedef struct sexpr_t sexpr_t;
typedef sexpr_t *expr;
typedef struct lispenv_t lispenv_t;
typedef lispenv_t *lisp;

typedef enum {
        /*S_INVALID, // zero should be an invalid type for calloc */
        S_NIL,                  /* 0:  () */
        S_TEE,                  /* 1:  t */
        S_CONS,                 /* 2:  cons list */
        S_STRING,               /* 3:  string */
        S_SYMBOL,               /* 4:  symbol */
        S_INTEGER,              /* 5:  integer */
        S_PRIMITIVE,            /* 6:  a primitive function */
        S_FILE,                 /* 7:  file I/O object */
        S_PROC,                 /* 8:  lambda procedure */
        S_QUOTE,                /* 9:  quoted expression */
        S_ERROR,                /* 10: error return and handling */
        S_LAST_TYPE             /* 11: not a type, just the last enum */
} sexpr_e;

typedef union {
        int32_t integer;
        char *string;
        char *symbol;
        struct sexpr_t *ptr;
        io *io;
        expr(*func) (expr args, lisp l);
} cell_union_t;

typedef struct atom_t {
        cell_union_t data;
        size_t len;
        sexpr_e type;
} atom_t;

/**sexpr module**/
struct sexpr_t { /** base type for our expressions */
        atom_t car;
        atom_t cdr;
        bool gc_mark;  /*the mark of the garbage collector */
};

/**lisp global environment struct**/
struct lispenv_t {/** a lisp environment */
        io *i;                  /* input */
        io *o;                  /* output */
        expr global; /*global key-val list*/
        expr env;
};

#ifdef __cplusplus
}
#endif
#endif
