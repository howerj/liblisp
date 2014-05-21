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

#define MAX_ALLOCS (1024u*1024u)

void *wmalloc(size_t size, io *e);
void *wcalloc(size_t num, size_t size, io *e);
void *wrealloc(void *ptr, size_t size, io *e);
void wfree(void * ptr, io *e);

expr gcmalloc(io *e);
expr gccalloc(io *e);
int  gcmark(expr root, io *e);
void gcsweep(io *e);

#endif
