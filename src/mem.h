/**
 *  @file           mem.h                                                    
 *  @brief          Memory wrapper and handling interface, header
 *  @author         Richard James Howe.                                      
 *  @copyright      Copyright 2013 Richard James Howe.                       
 *  @license        GPL v3.0                                                 
 *  @email          howe.r.j.89@gmail.com                                    
 **/

#ifndef MEM_H
#define MEM_H

#include "io.h"

void *wmalloc(size_t size, io *e);
void *wcalloc(size_t num, size_t size, io *e);
void *wrealloc(void *ptr, size_t size, io *e);
void wfree(void * ptr, io *e);

#endif
