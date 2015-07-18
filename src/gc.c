/**
 *  @file           gc.c
 *  @brief          A precise garbage collector
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 *  Garabage collection and error handling on Out-Of-Memory errors should go here.
 *
 *  @todo Implement error handling
 *  @todo Extensive testing using Valgrind
 *  @todo This currently will not work if multiple instances of the the library
 *        are running at the same time, the garbage collector needs to run in its
 *        own local context.
 *  @todo Add functions for monitoring the number of allocations there have been
 *        and for returning that number so the rest of the program can figure out
 *        when to collect.
 *  @todo Change heap into growable array, not linked list. Reimplement garbage
 *        collection.
 */

#include "gc.h"
#include "mem.h"

struct heap { expr x; struct heap *next; };  /* struct of all allocated objs */
static struct heap heaplist = {NULL,NULL};   /* initial element of alloc list */
static struct heap *heaphead = &heaplist;
static void gcinner(expr x);

/*** interface functions *****************************************************/

/**
 *  @brief          wrapper around calloc for garbage collection
 *  @return         pointer to newly allocated storage on sucess, which
 *                  is zeroed, exits program on failure!
 **/
expr gc_calloc(sexpr_e init)
{
        expr v;
        struct heap *nextheap;
        v = mem_calloc(sizeof(*v));
        nextheap = mem_calloc(sizeof(struct heap));
        nextheap->x = v;
        heaphead->next = nextheap;
        heaphead = nextheap;
        v->type = init;
        return v;
}

/**
 *  @brief          Given a root structure, mark all accessible
 *                  objects in the tree so they do not get garbage
 *                  collected
 *  @param          root root tree to mark
 *  @return         false == root was not marked, and now is
 **/
int gc_mark(expr root)
{
        if(NULL == root)
                return false;
        root->gc_mark = true;

        switch(root->type){
        case S_NIL:     case S_TEE:     case S_STRING:  
        case S_SYMBOL:  case S_INTEGER: case S_PRIMITIVE: 
        case S_FILE:    case S_ERROR:   case S_HASH:
        case S_LISP_ENV:
                break;
        case S_PROC: /*needs special handling*/ break;
        case S_QUOTE: gc_mark(root->data.quoted); break;
        case S_CONS: 
                      do{
                              gc_mark(root->data.cons[0]);
                      } while((root = root->data.cons[1]));
                      break;
        case S_LAST_TYPE: /*fall through, not a type*/
        default:
                IO_REPORT("Not a valid type");
                exit(EXIT_FAILURE);
        }
        return false;
}

/**
 *  @brief          Sweep all unmarked objects.
 *  @return         void
 **/
void gc_sweep(void)
{
#if 0
        /**@todo this really needs cleaning up**/
        struct heap *ll, *pll;
        if (NULL == heaplist.next)      /*pass first element, do not collect element */
                return;
        for (ll = heaplist.next, pll = &heaplist; ll && (ll != heaphead);) {
                if(NULL == ll->x)
                        return;
                if (true == ll->x->gc_mark) {
                        ll->x->gc_mark = false;
                        pll = ll;
                        ll = ll->next;
                } else {
                        gcinner(ll->x);
                        ll->x = NULL;
                        pll->next = ll->next;
                        mem_free(ll);
                        ll = pll->next;
                }
        }
        if ((heaphead != &heaplist) && (NULL != heaphead->x)) {
                if (true == heaphead->x->gc_mark) {
                        ll->x->gc_mark = false;
                } else {
                        gcinner(heaphead->x);
                        heaphead->x = NULL;
                        mem_free(heaphead);
                        heaphead = pll;
                }
        }
#endif
}

/*****************************************************************************/

/**
 *  @brief          Frees expressions and deals with each type of atoms
 *                  destruction.
 *  @param          x     expression to print
 *  @return         void
 **/
static void gcinner(expr x)
{
        if((NULL == x) || x->gc_nocollect)
                return;
        switch(x->type){
        case S_NIL:       case S_TEE:   case S_INTEGER: /*fall through*/ 
        case S_PRIMITIVE: case S_QUOTE: case S_CONS:    /*fall through*/
        case S_LISP_ENV:  mem_free(x); break;
        case S_STRING:    mem_free(x->data.string); mem_free(x); break;
        case S_SYMBOL:    mem_free(x->data.symbol); mem_free(x); break;
        case S_FILE:      break;
        case S_PROC:      break;
        case S_ERROR:     break;
        case S_HASH:      break;
        case S_LAST_TYPE: /*fall through, not a valid type*/
        default:
                IO_REPORT("Not a valid type");
                exit(EXIT_FAILURE);
        }
}

/*****************************************************************************/
