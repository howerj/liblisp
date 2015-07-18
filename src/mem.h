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

#ifdef __cplusplus
extern "C" {
#endif

#include "io.h"

#define MAX_ALLOCS ((signed int)(1024*1024))

void mem_set_debug(bool flag);

void *mem_malloc(size_t size);
void *mem_calloc(size_t size);
void *mem_realloc(void *ptr, size_t size);
void mem_free(void *ptr);
char *mem_strdup(const char *s);

#ifdef __cplusplus
}
#endif
#endif
