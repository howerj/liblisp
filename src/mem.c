/**
 *  @file           mem.c
 *  @brief          Memory allocation wrappers and handling
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v3.0
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 *  Wrappers for allocation. Garabage collection and error handling on
 *  Out-Of-Memory errors should go here as well to keep things out of
 *  the way.
 *
 *  @todo The actual garbage collection stuff.
 *  @todo If an allocation fails, garbage should be collected, then an
 *        allocation reattempted, if it fails again it should abort.
 *  @todo Debug functions; maximum allocations / deallocations for example
 *  @todo Remove instances of fprintf
 */

#include "type.h"
#include "io.h"
#include "mem.h"
#include <stdlib.h> /** malloc(), calloc(), realloc(), free(), exit() */

/**** malloc wrappers ********************************************************/

/**
 *  @brief          wrapper around malloc
 *  @param          size size of desired memory block in bytes
 *  @param          e    error output stream
 *  @return         pointer to newly allocated storage on sucess, exits
 *                  program on failure!
 **/
void *wmalloc(size_t size, io *e){
  void* v;
  v = malloc(size);
  if(NULL == v){
    if(NULL == e){
      fprintf(stderr, "malloc failed and *e == NULL\n");
    } else {
    }
    exit(EXIT_FAILURE);
  }
  return v;
}

/**
 *  @brief          wrapper around calloc
 *  @param          num  number of elements to allocate
 *  @param          size size of elements to allocate
 *  @param          e    error output stream
 *  @return         pointer to newly allocated storage on sucess, which
 *                  is zeroed, exits program on failure!
 **/
void *wcalloc(size_t num, size_t size, io *e){
  void* v;
  v = calloc(num,size);
  if(NULL == v){
    if(NULL == e){
      fprintf(stderr, "calloc failed and *e == NULL\n");
    } else {
    }
    exit(EXIT_FAILURE);
  }
  return v;
}

/**
 *  @brief          wrapper around realloc
 *  @param          size size of desired memory block in bytes
 *  @param          ptr  existing memory block to resize
 *  @param          size size of desired memory block in bytes
 *  @param          e    error output stream
 *  @return         pointer to newly resized storage on success, 
 *                  exits program on failure!
 **/
void *wrealloc(void *ptr, size_t size, io *e){
  void* v;
  v = realloc(ptr,size);
  if(NULL == v){
    if(NULL == e){
      fprintf(stderr, "realloc failed and *e == NULL\n");
    } else {
    }
    exit(EXIT_FAILURE);
  }
  return v;
}

/**
 *  @brief          wrapper around free
 *  @param          ptr  pointer to free; make sure its not NULL!
 *  @param          e    error output stream
 *  @return         void
 **/
void wfree(void *ptr, io *e){
  if(NULL == e){ /* *I* should not be passing null to free */
    fprintf(stderr, "free failed and *e == NULL\n");
    exit(EXIT_FAILURE);
  }
  free(ptr);
}

/*****************************************************************************/
