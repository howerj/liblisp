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
 *
 *  @todo The actual garbage collection stuff.
 *  @todo If an allocation fails, garbage should be collected, then an
 *        allocation reattempted, if it fails again it should abort.
 *  @todo Debug functions; maximum allocations / deallocations for example
 *  @todo Remove instances of fprintf
 *  @todo Make the interfaces better so it is clear that expressions or
 *        pointers of expressions are being allocated.
 */

#include "type.h"
#include "io.h"
#include "mem.h"
#include <stdlib.h> /** malloc(), calloc(), realloc(), free(), exit() */

struct heap{
  expr x;
  struct heap *next;
};

static unsigned int alloccounter = 0;
static struct heap heaplist = {NULL,NULL};
static struct heap *heaphead = &heaplist;

static void gcinner(expr x, io *e);

/*** interface functions *****************************************************/

/**
 *  @brief          wrapper around malloc 
 *  @param          size size of desired memory block in bytes
 *  @param          e    error output stream
 *  @return         pointer to newly allocated storage on sucess, exits
 *                  program on failure!
 **/
void *wmalloc(size_t size, io *e){
  void* v;
  if(MAX_ALLOCS < alloccounter++){
    fprintf(stderr,"too many mallocs\n");
    exit(EXIT_FAILURE);
  }
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
  if(MAX_ALLOCS < alloccounter++){
    fprintf(stderr,"too many mallocs\n");
    exit(EXIT_FAILURE);
  }
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
 *  @brief          wrapper around realloc, no gc necessary
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
 *  @brief          wrapper around malloc for garbage collection
 *  @param          size size of desired memory block in bytes
 *  @param          e    error output stream
 *  @return         pointer to newly allocated storage on sucess, exits
 *                  program on failure!
 **/
expr gcmalloc(io *e){
  void* v;
  v = wmalloc(sizeof(struct sexpr_t),e);
  return v;
}

/**
 *  @brief          wrapper around calloc for garbage collection
 *  @param          num  number of elements to allocate
 *  @param          size size of elements to allocate
 *  @param          e    error output stream
 *  @return         pointer to newly allocated storage on sucess, which
 *                  is zeroed, exits program on failure!
 **/
expr gccalloc(io *e){
  expr v;
  struct heap *nextheap;
  v = wcalloc(1,sizeof(struct sexpr_t), e);
  nextheap = wcalloc(1,sizeof(struct heap), e);
  nextheap->x = v;
  heaphead->next = nextheap;
  heaphead = nextheap;
  return v;
}

/**
 *  @brief          wrapper around free
 *  @param          ptr  pointer to free; make sure its not NULL!
 *  @param          e    error output stream
 *  @return         void
 **/
void wfree(void *ptr, io *e){
  alloccounter--;
  if(NULL == e){ /* *I* should not be passing null to free */
    fprintf(stderr, "free failed and *e == NULL\n");
    exit(EXIT_FAILURE);
  }
  free(ptr);
}

/**
 *  @brief          Given a root structure, mark all accessible
 *                  objects in the tree so they do not get garbage
 *                  collected
 *  @param          root root tree to mark
 *  @param          e    error output stream
 *  @return         false == root was not marked, and now is, 
 *                  true == root was already marked
 **/
int gcmark(expr root, io *e){
  if(NULL == root)
    return false;

  if(true == root->gcmark)
    return true;

  root->gcmark = true;

  switch(root->type){
    case S_LIST:
      {
        unsigned int i;
        for(i = 0; i < root->len; i++)
          gcmark(root->data.list[i],e);
      }
      break;
    case S_PRIMITIVE:
      break;
    case S_PROC:
      /*@todo Put the S_PROC structure into type.h**/
      gcmark(root->data.list[0],e);
      gcmark(root->data.list[1],e);
      gcmark(root->data.list[2],e);
    case S_NIL:
    case S_TEE:
    case S_STRING:
    case S_SYMBOL:
    case S_INTEGER:
    case S_FILE:
      break;
    default:
      fprintf(stderr,"unmarkable type\n");
      exit(EXIT_FAILURE);
  }
  return false;
}

/**
 *  @brief          Sweep all unmarked objects.
 *  @param          e    error output stream
 *  @return         void
 **/
void gcsweep(io *e){
  struct heap *ll;
  if(NULL == heaplist.next) /*pass first element, do not collect element*/
    return;
  for(ll = heaplist.next; ll != heaphead; ll = ll->next){
    if(true == ll->x->gcmark)
      ll->x->gcmark = false;
    else
      printf("freed %p\n",ll->x);
  }
}

/*****************************************************************************/

/**
 *  @brief          Frees expressions
 *  @param          x     expression to print
 *  @param          e     error output stream
 *  @return         void
 **/
static void gcinner(expr x, io *e){
  if (NULL==x)
    return;

  switch (x->type) {
  case S_PRIMITIVE:
  case S_INTEGER:
  case S_TEE:
  case S_NIL:
      wfree(x,e);
      break;
  case S_LIST:
    wfree(x->data.list,e);
    wfree(x,e);
    return;
  case S_SYMBOL:
  case S_STRING:
    wfree(x->data.string,e);
    wfree(x,e);
    return;
  case S_PROC:
  case S_FILE: /** @todo implement file support **/
    report("UNIMPLEMENTED (TODO)");
    break;
  default: /* should never get here */
    report("free: not a known 'free-able' type");
    exit(EXIT_FAILURE);
    return;
  }
}

/*****************************************************************************/
