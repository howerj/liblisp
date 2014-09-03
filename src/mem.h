/**
 *  @file           mem.h
 *  @brief          Memory wrapper and handling interface, header
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v2.0 or later version
 *  @email          howe.r.j.89@gmail.com
 **/

#ifndef MEM_H
#define MEM_H

#include "io.h"

#define MAX_ALLOCS ((signed int)(1024*1024))

void mem_set_debug(bool flag);

void *mem_malloc(size_t size, io * e);
void *mem_calloc(size_t num, size_t size, io * e);
void *mem_realloc(void *ptr, size_t size, io * e);
void mem_free(void *ptr, io * e);

expr mem_gc_malloc(io * e);
expr mem_gc_calloc(io * e);
int mem_gc_mark(expr root, io * e);
void mem_gc_sweep(io * e);

#endif
