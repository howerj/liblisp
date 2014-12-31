/**
 *  @file           gc.c
 *  @brief          A precise garbage collector
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 *  Garabage collection and error handling on
 *  Out-Of-Memory errors should go here.
 */

#include "gc.h"
#include "mem.h"

struct heap {
        expr x;
        struct heap *next;
};

static struct heap heaplist = { NULL, NULL };
static struct heap *heaphead = &heaplist;

static void gcinner(expr x);

/*** interface functions *****************************************************/

/**
 *  @brief          wrapper around malloc for garbage collection
 *  @return         pointer to newly allocated storage on sucess, exits
 *                  program on failure!
 **/
expr gc_malloc(void)
{
        void *v;
        v = mem_malloc(sizeof(struct sexpr_t));
        return v;
}

/**
 *  @brief          wrapper around calloc for garbage collection
 *  @return         pointer to newly allocated storage on sucess, which
 *                  is zeroed, exits program on failure!
 **/
expr gc_calloc(void)
{
        expr v;
        struct heap *nextheap;
        v = mem_calloc(1, sizeof(struct sexpr_t));
        nextheap = mem_calloc(1, sizeof(struct heap));
        nextheap->x = v;
        heaphead->next = nextheap;
        heaphead = nextheap;
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
        case S_FILE:    case S_ERROR: 
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
        /**@todo this really needs cleaning up**/
        struct heap *ll, *pll;
        if (NULL == heaplist.next)      /*pass first element, do not collect element */
                return;
        for (ll = heaplist.next, pll = &heaplist; ll != heaphead;) {
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
}

/*****************************************************************************/

/**
 *  @brief          Frees expressions
 *  @param          x     expression to print
 *  @return         void
 **/
static void gcinner(expr x)
{
        if(NULL == x)
                return;

        switch(x->type){
        case S_NIL: break;
        case S_TEE: break;
        case S_STRING: break;
        case S_SYMBOL: break;
        case S_INTEGER: break;
        case S_PRIMITIVE: break;
        case S_FILE: break;
        case S_PROC: break;
        case S_QUOTE: break;
        case S_ERROR: break;
        case S_CONS: break;
        case S_LAST_TYPE: /*fall through, not a type*/
        default:
                IO_REPORT("Not a valid type");
                exit(EXIT_FAILURE);
        }
}

/*****************************************************************************/
