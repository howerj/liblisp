/**
 *  @file           mem.c
 *  @brief          Memory allocation wrappers and handling
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 */

#include <stdint.h>  /* intX_t */
#include <stdio.h>   /* FILE* */
#include <stdlib.h>  /* malloc(), calloc(), realloc(), free(), exit() */
#include <stdbool.h> /* bool */
#include "io.h"
#include "mem.h"

/* 
 * alloccounter is used to implement a crude way of making sure the program
 * fails instead of trying allocating everything when we do something wrong,
 * instead of just exiting, perhaps we should restart gracefully?
 */
static signed int alloccounter = 0;
static bool debug_f = false;

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
 *  @brief          wrapper around free
 *  @param          ptr  pointer to free
 *  @param          e    error output stream
 *  @return         void
 **/
void mem_free(void *ptr, io * e)
{
        if (NULL != ptr) {
                alloccounter--;
                if ((true == debug_f) && (NULL !=e)) {
                        io_puts("mem_free:", e, e);
                        io_printd(alloccounter, e, e);
                        io_putc('\n', e, e);
                }
        }
        free(ptr);
        ptr = NULL;
}

/*****************************************************************************/
