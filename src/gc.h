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

#include "io.h"
#include "type.h"

expr mem_gc_malloc(io * e);
expr mem_gc_calloc(io * e);
int mem_gc_mark(expr root, io * e);
void mem_gc_sweep(io * e);

#endif
