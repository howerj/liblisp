/**
 *  @file           mem.c
 *  @brief          Memory allocation wrappers and handling
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *  @details
 **/

#include "mem.h"
#include <string.h>
#include <stdlib.h>

/* 
 * mem_alloc_counter is used to implement a crude way of making sure the program
 * fails instead of trying allocating everything when we do something wrong,
 * instead of just exiting, perhaps we should restart gracefully?
 */
static int32_t mem_alloc_counter = 0;
static bool mem_debug_f = false;

/*** interface functions *****************************************************/

/**
 *  @brief          Set debugging on.
 *  @param          flag 
 *  @return         void
 **/
void mem_set_debug(bool flag)
{
        mem_debug_f = flag;
}

/**
 *  @brief          wrapper around malloc 
 *  @param          size size of desired memory block in bytes
 *  @return         pointer to newly allocated storage on sucess, exits
 *                  program on failure!
 **/
void *mem_malloc(size_t size)
{
        void *v;
        if (0 == size)
                return NULL;

        if (MAX_ALLOCS < mem_alloc_counter++) {
                REPORT("too many mallocs");
                exit(EXIT_FAILURE);
        }
        if (true == mem_debug_f)
                io_printer(io_get_error_stream(), "(mem_malloc %d)\n", mem_alloc_counter);

        v = malloc(size);
        if (NULL == v) {
                REPORT("malloc failed");
                exit(EXIT_FAILURE);
        }
        return v;
}

/**
 *  @brief          wrapper around calloc
 *  @param          num  number of elements to allocate
 *  @param          size size of elements to allocate
 *  @return         pointer to newly allocated storage on sucess, which
 *                  is zeroed, exits program on failure!
 **/
void *mem_calloc(size_t num, size_t size)
{
        void *v;
        if (MAX_ALLOCS < mem_alloc_counter++) {
                REPORT("too many mallocs");
                exit(EXIT_FAILURE);
        }

        if (true == mem_debug_f)
                io_printer(io_get_error_stream(), "(mem_calloc %d)\n", mem_alloc_counter);

        v = calloc(num, size);
        if (NULL == v) {
                REPORT("calloc failed");
                exit(EXIT_FAILURE);
        }
        return v;
}

/**
 *  @brief          wrapper around realloc, no gc necessary
 *  @param          size size of desired memory block in bytes
 *  @param          ptr  existing memory block to resize
 *  @param          size size of desired memory block in bytes
 *  @return         pointer to newly resized storage on success, 
 *                  exits program on failure!
 **/
void *mem_realloc(void *ptr, size_t size)
{
        void *v;
        if (0 == size) {        /*acts as free */
                mem_alloc_counter--;
                free(ptr);
        }
        v = realloc(ptr, size);
        if (NULL == ptr)        /*acts as malloc */
                mem_alloc_counter++;

        if (NULL == v) {
                REPORT("realloc failed");
                exit(EXIT_FAILURE);
        }
        return v;
}

/**
 *  @brief          wrapper around free
 *  @param          ptr  pointer to free
 *  @return         void
 **/
void mem_free(void *ptr)
{
        if ((NULL != ptr) && (true == mem_debug_f))
                io_printer(io_get_error_stream(), "(mem_free %d)\n", mem_alloc_counter--);
        free(ptr);
        ptr = NULL;
}

/**
 *  @brief          Duplicate a string, allocating memory for it.
 *  @param          s           string to duplicate
 *  @return         char*       duplicate string
 **/
char *mem_strdup(const char *s)
{
        char *ns;
        if (NULL == s) {
                REPORT("mem_strdup passed NULL");
                abort();
        }
        ns = mem_malloc(sizeof(char) * (strlen(s) + 1));
        strcpy(ns, s);
        return ns;
}

/*****************************************************************************/
