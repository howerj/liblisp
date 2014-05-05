#ifndef MEM_H
#define MEM_H

#include "io.h"

void *wmalloc(size_t size, io *e);
void *wcalloc(size_t num, size_t size, io *e);
void *wrealloc(void *ptr, size_t size, io *e);
void wfree(void * ptr, io *e);

#endif
