/**
 *  @file           mem.c
 *  @brief          Memory allocation wrappers and handling
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v2.0 or later version
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 *  Wrappers for allocation. Garabage collection and error handling on
 *  Out-Of-Memory errors should go here as well to keep things out of
 *  the way.
 *
 *  @todo If an allocation fails, garbage should be collected, then an
 *        allocation reattempted, if it fails again it should abort.
 *  @todo Debug functions; a poor-mans version of valgrind.
 */

#include "type.h"
#include "io.h"
#include "mem.h"
#include <stdlib.h> /** malloc(), calloc(), realloc(), free(), exit() */

struct heap {
        expr x;
        struct heap *next;
};

/* 
 * alloccounter is used to implement a crude way of making sure the program
 * fails instead of trying allocating everything when we do something wrong,
 * instead of just exiting, perhaps we should restart gracefully?
 */
static signed int alloccounter = 0;
static bool debug_f = false;
static struct heap heaplist = { NULL, NULL };

static struct heap *heaphead = &heaplist;

static void gcinner(expr x, io * e);

/*** interface functions *****************************************************/

/**
 *  @brief          Set debugging on.
 *  @param          flag 
 *  @return         void
 **/
void mem_set_debug(bool flag)
{
        debug_f = flag;
}

/**
 *  @brief          wrapper around malloc 
 *  @param          size size of desired memory block in bytes
 *  @param          e    error output stream
 *  @return         pointer to newly allocated storage on sucess, exits
 *                  program on failure!
 **/
void *mem_malloc(size_t size, io * e)
{
        void *v;
        if (0 == size)
                return NULL;

        if (MAX_ALLOCS < alloccounter++) {
                report("too many mallocs", e);
                exit(EXIT_FAILURE);
        }
        if (true == debug_f) {
                io_puts("mem_malloc:", e, e);
                io_printd(alloccounter, e, e);
                io_putc('\n', e, e);
        }

        v = malloc(size);
        if (NULL == v) {
                report("malloc failed", e);
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
void *mem_calloc(size_t num, size_t size, io * e)
{
        void *v;
        if (MAX_ALLOCS < alloccounter++) {
                report("too many mallocs", e);
                exit(EXIT_FAILURE);
        }

        if (true == debug_f) {
                io_puts("mem_calloc:", e, e);
                io_printd(alloccounter, e, e);
                io_putc('\n', e, e);
        }

        v = calloc(num, size);
        if (NULL == v) {
                report("calloc failed", e);
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
void *mem_realloc(void *ptr, size_t size, io * e)
{
        void *v;
        if (0 == size) {        /*acts as free */
                alloccounter--;
                free(ptr);
        }
        v = realloc(ptr, size);
        if (NULL == ptr) {      /*acts as malloc */
                alloccounter++;
        }
        if (NULL == v) {
                report("realloc failed", e);
                exit(EXIT_FAILURE);
        }
        return v;
}

/**
 *  @brief          wrapper around malloc for garbage collection
 *  @param          e    error output stream
 *  @return         pointer to newly allocated storage on sucess, exits
 *                  program on failure!
 **/
expr mem_gc_malloc(io * e)
{
        void *v;
        v = mem_malloc(sizeof(struct sexpr_t), e);
        return v;
}

/**
 *  @brief          wrapper around calloc for garbage collection
 *  @param          e    error output stream
 *  @return         pointer to newly allocated storage on sucess, which
 *                  is zeroed, exits program on failure!
 **/
expr mem_gc_calloc(io * e)
{
        expr v;
        struct heap *nextheap;
        v = mem_calloc(1, sizeof(struct sexpr_t), e);
        nextheap = mem_calloc(1, sizeof(struct heap), e);
        nextheap->x = v;
        heaphead->next = nextheap;
        heaphead = nextheap;
        return v;
}

/**
 *  @brief          wrapper around free
 *  @param          ptr  pointer to free
 *  @param          e    error output stream
 *  @return         void
 **/
void mem_free(void *ptr, io * e)
{
        if (NULL != ptr) {
                alloccounter--;
                if (true == debug_f) {
                        io_puts("mem_free:", e, e);
                        io_printd(alloccounter, e, e);
                        io_putc('\n', e, e);
                }
        }
        free(ptr);
        ptr = NULL;
}

/**
 *  @brief          Given a root structure, mark all accessible
 *                  objects in the tree so they do not get garbage
 *                  collected
 *  @param          root root tree to mark
 *  @param          e    error output stream
 *  @return         false == root was not marked, and now is
 **/
int mem_gc_mark(expr root, io * e)
{
        if (NULL == root)
                return false;

        root->mem_gc_mark = true;

        switch (root->type) {
        case S_LIST:
                {
                        unsigned int i;
                        for (i = 0; i < root->len; i++)
                                mem_gc_mark(root->data.list[i], e);
                }
                return false;
        case S_PROC:
                /*@todo Put the S_PROC structure into type.h* */
                mem_gc_mark(root->data.list[0], e);
                mem_gc_mark(root->data.list[1], e);
                mem_gc_mark(root->data.list[2], e);
                return false;
        case S_PRIMITIVE:
        case S_NIL:
        case S_TEE:
        case S_STRING:
        case S_SYMBOL:
        case S_INTEGER:
        case S_ERROR:
        case S_FILE:
                break;
        default:
                fprintf(stderr, "unmarkable type\n");
                exit(EXIT_FAILURE);
        }
        return false;
}

/**
 *  @brief          Sweep all unmarked objects.
 *  @param          e    error output stream
 *  @return         void
 **/
void mem_gc_sweep(io * e)
{
  /**@todo this really needs cleaning up**/
        struct heap *ll, *pll;
        if (NULL == heaplist.next)      /*pass first element, do not collect element */
                return;
        for (ll = heaplist.next, pll = &heaplist; ll != heaphead;) {
                if (true == ll->x->mem_gc_mark) {
                        ll->x->mem_gc_mark = false;
                        pll = ll;
                        ll = ll->next;
                } else {
                        gcinner(ll->x, e);
                        ll->x = NULL;
                        pll->next = ll->next;
                        mem_free(ll, e);
                        ll = pll->next;
                }
        }
        if ((heaphead != &heaplist) && (NULL != heaphead->x)) {
                if (true == heaphead->x->mem_gc_mark) {
                        ll->x->mem_gc_mark = false;
                } else {
                        gcinner(heaphead->x, e);
                        heaphead->x = NULL;
                        mem_free(heaphead, e);
                        heaphead = pll;
                }
        }
}

/*****************************************************************************/

/**
 *  @brief          Frees expressions
 *  @param          x     expression to print
 *  @param          e     error output stream
 *  @return         void
 **/
static void gcinner(expr x, io * e)
{
        if (NULL == x)
                return;

        switch (x->type) {
        case S_TEE:
        case S_NIL:
        case S_INTEGER:
        case S_PRIMITIVE:
                mem_free(x, e);
                break;
        case S_PROC:
                mem_free(x->data.list, e);
                mem_free(x, e);
                break;
        case S_LIST:
                mem_free(x->data.list, e);
                mem_free(x, e);
                return;
        case S_SYMBOL:
                mem_free(x->data.symbol, e);
                mem_free(x, e);
                return;
        case S_STRING:
                mem_free(x->data.string, e);
                mem_free(x, e);
                return;
        case S_ERROR:
                /** @todo implement error support **/
                mem_free(x, e);
                return;
        case S_FILE:
               /** @todo implement file support **/
                report("UNIMPLEMENTED (TODO)", e);
                break;
        default:               /* should never get here */
                report("free: not a known 'free-able' type", e);
                exit(EXIT_FAILURE);
                return;
        }
}

/*****************************************************************************/
