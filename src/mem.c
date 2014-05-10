/**
 *  @file           mem.c
 *  @brief          Memory allocation wrappers and handling
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v3.0
 *  @email          howe.r.j.89@gmail.com
 **/

/*
 *  Wrappers for allocation. Garabage collection and error handling on
 *  Out-Of-Memory errors should go here as well to keep things out of
 *  the way.
 *  TODO:
 *  * The actual garbage collection stuff.
 *  * If an allocation fails, garbage should be collected, then an
 *    allocation reattempted, if it fails again it should abort.
 *  * Debug functions; maximum allocations / deallocations for example
 */

#include "type.h"
#include "io.h"
#include "mem.h"
#include <stdlib.h> /** exit() */

/**** malloc wrappers ********************************************************/

void *wmalloc(size_t size, io *e){
  void* v;
  v = malloc(size);
  if(NULL == v){
    if(NULL == e){
      fprintf(stderr, "malloc failed and *e == NULL\n");
    } else {
    }
    exit(-1);
  }
  return v;
}

void *wcalloc(size_t num, size_t size, io *e){
  void* v;
  v = calloc(num,size);
  if(NULL == v){
    if(NULL == e){
      fprintf(stderr, "calloc failed and *e == NULL\n");
    } else {
    }
    exit(-1);
  }
  return v;
}

void *wrealloc(void *ptr, size_t size, io *e){
  void* v;
  v = realloc(ptr,size);
  if(NULL == v){
    if(NULL == e){
      fprintf(stderr, "realloc failed and *e == NULL\n");
    } else {
    }
    exit(-1);
  }
  return v;
}

void wfree(void *ptr, io *e){
  if(NULL == e){ /* *I* should not be passing null to free */
    fprintf(stderr, "free failed and *e == NULL\n");
    exit(-1);
  }
  free(ptr);
}

/*****************************************************************************/
