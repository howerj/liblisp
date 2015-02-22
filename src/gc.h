/**
 *  @file           gc.h
 *  @brief          Precise garbage collection, header only
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 **/

#ifndef GC_H
#define GC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "io.h"
#include "type.h"

expr gc_malloc(void);
expr gc_calloc(sexpr_e init);
int  gc_mark(expr root);
void gc_sweep(void);

#ifdef __cplusplus
}
#endif
#endif
