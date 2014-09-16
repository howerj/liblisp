/**
 *  @file           mem.h
 *  @brief          Memory wrapper and handling interface, header
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 **/

#ifndef MEM_H
#define MEM_H

#define MAX_ALLOCS ((signed int)(1024*1024))

#include "io.h"
#include "type.h"

void mem_set_debug(bool flag);

void *mem_malloc(size_t size, io * e);
void *mem_calloc(size_t num, size_t size, io * e);
void *mem_realloc(void *ptr, size_t size, io * e);
void mem_free(void *ptr, io * e);

#endif
