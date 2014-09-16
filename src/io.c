/**
 *  @file           io.c
 *  @brief          I/O redirection and wrappers
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *
 *  @todo Implement set_error_stream instead of passing the error
 *        constantly to functions like io_puts
 *
 *  It is possible to implement something similar using 'setbuf' and
 *  family from <stdio.h> and perhaps more cleanly and efficiently as
 *  well. However it would not *quite* replicate what I want and such
 *  functionality would have to be wrapped up anyway.
 *
 **/

#include <assert.h>     /*assert*/
#include <string.h>     /*strlen,memset*/
#include <stdio.h>      /*...*/
#include <stdlib.h>     /*exit*/
#include <stdint.h>
#include "io.h"

typedef enum{
        false,
        true
} bool;

/**I/O abstraction structure**/
struct io {
        enum iotype {
                IO_INVALID,   /* error on incorrectly set up I/O */
                IO_FILE_IN,   /* read from file */
                IO_FILE_OUT,  /* write to file */
                IO_STRING_IN, /* read from a string */
                IO_STRING_OUT /* write to a string, if you want */
        }type;

        union {
                FILE *file;
                char *string;
        } ptr; 

        size_t position;        /* position in string */
        size_t max;             /* max string length, if known */
        char c;                 /* character store for io_ungetc() */
        unsigned int ungetc;            /* true if we have ungetc'ed a character */
};

/**** I/O functions **********************************************************/

/**
 *  @brief          Set input wrapper to read from a string
 *  @param          i           input stream
 *  @param          s           string to read from
 *  @return         void
 **/
void io_string_in(io *i, char *s){
        assert((NULL != i) && (NULL != s));
        memset(i, 0, sizeof(*i));
        i->type         = IO_STRING_IN;
        i->ptr.string   = s;
        i->max          = strlen(s);
        return;
}

/**
 *  @brief          Set output stream to point to a string
 *  @param          o           output stream
 *  @param          s           string to write to
 *  @return         void
 **/
void io_string_out(io *o, char *s){
        assert((NULL != o) && (NULL != s));
        memset(o, 0, sizeof(*o));
        o->type         = IO_STRING_OUT;
        o->ptr.string   = s;
        o->max          = strlen(s);
        return;
}

/**
 *  @brief          Attempts to open up a file called file_name for
 *                  reading, setting the input stream wrapper to read
 *                  from it.
 *  @param          i           input stream
 *  @param          file_name   File to open and read from
 *  @return         FILE*       Initialized FILE*, or NULL on failure
 **/
FILE *io_filename_in(io *i, char *file_name){
        assert((NULL != i)&&(NULL != file_name));
        memset(i, 0, sizeof(*i));
        i->type         = IO_FILE_IN;
        if(NULL == (i->ptr.file = fopen(file_name, "r")))
                return NULL;
        return i->ptr.file;
}

/**
 *  @brief          Attempt to open file_name for writing, setting the
 *                  output wrapper to use it
 *  @param          o           output stream
 *  @param          file_name   file to open
 *  @return         FILE*       Initialized file pointer, or NULL on failure
 **/
FILE *io_filename_out(io *o, char *file_name){
        assert((NULL != o)&&(NULL != file_name));
        memset(o, 0, sizeof(*o));
        o->type         = IO_FILE_OUT;
        if(NULL == (o->ptr.file = fopen(file_name, "w")))
                return NULL;
        return o->ptr.file;
}

/**
 *  @brief          Set input stream wrapper to point to a FILE*
 *  @param          i           input stream
 *  @param          file        file to read from
 *  @return         void
 **/
void io_file_in(io *i, FILE* file){
        assert((NULL != i)&&(NULL != file));
        memset(i, 0, sizeof(*i));
        i->type         = IO_FILE_IN;
        i->ptr.file     = file;
        return;
}

/**
 *  @brief          Set an output stream wrapper to use a FILE*
 *  @param          o           output stream
 *  @param          file        FILE* to write to
 *  @return         void
 **/
void io_file_out(io *o, FILE* file){
        assert((NULL != o));
        assert((NULL != file));
        memset(o, 0, sizeof(*o));
        o->type         = IO_FILE_OUT;
        o->ptr.file     = file;
        return;
}

/**
 *  @brief          Flush and close an input or output stream
 *  @param          ioc         Input or output stream to close
 *  @return         void
 **/
void io_file_close(io *ioc){
        assert(NULL != ioc);

        if((IO_FILE_IN == ioc->type) || (IO_STRING_OUT == ioc->type)){
                if(NULL != ioc->ptr.file){
                        fflush(ioc->ptr.file);
                        fclose(ioc->ptr.file);
                        ioc->ptr.file = NULL;
                }
        }
        return;
}

/**
 *  @brief          Return the size of the 'io' struct, this is an incomplete
 *                  type to the outside world.
 *  @param          void
 *  @return         size_t size of io struct which is hidden from the outside
 **/
size_t io_sizeof_io(void){
        return sizeof(io);
}

/**
 *  @brief          wrapper around putc; redirect output to a file or string
 *  @param          c   output this character
 *  @param          o   output stream
 *  @param          e   error output stream
 *  @return         EOF on failure, character to output on success
 **/
int io_putc(char c, io * o, io * e)
{
        NULLCHK(o, e);
        NULLCHK(o->ptr.file, e);

        if (IO_FILE_OUT == o->type) {
                return fputc(c, o->ptr.file);
        } else if (IO_STRING_OUT == o->type) {
                if (o->position < o->max) {
                        o->ptr.string[o->position++] = c;
                        return c;
                } else {
                        return EOF;
                }
        } else {
                /*programmer error; some kind of error reporting would be nice */
                exit(EXIT_FAILURE);
        }
        return EOF;
}

/**
 *  @brief          wrapper around io_putc; get input from file or string
 *  @param          i input stream
 *  @param          e error output stream
 *  @return         EOF on failure, character input on success
 **/
int io_getc(io * i, io * e)
{
        NULLCHK(i, e);
        NULLCHK(i->ptr.file, e);

        if (true == i->ungetc) {
                i->ungetc = false;
                return i->c;
        }

        if (IO_FILE_IN == i->type) {
                return fgetc(i->ptr.file);
        } else if (IO_STRING_IN == i->type) {
                return (i->ptr.string[i->position]) ? i->ptr.string[i->position++] : EOF;
        } else {
                /*programmer error; some kind of error reporting would be nice */
                exit(EXIT_FAILURE);
        }
        return EOF;
}

/**
 *  @brief          wrapper around ungetc; unget from to file or string
 *  @param          c character to put back
 *  @param          i input stream to put character back into
 *  @param          e error output stream
 *  @return         EOF if failed, character we put back if succeeded.
 **/
int io_ungetc(char c, io * i, io * e)
{
        NULLCHK(i, e);
        NULLCHK(i->ptr.file, e);
        if (true == i->ungetc) {
                return EOF;
        }
        i->c = c;
        i->ungetc = true;
        return c;
}

/**
 *  @brief          wrapper to print out a number; this should be rewritten
 *                  to avoid using fprintf and sprintf 
 *  @param          d integer to print out
 *  @param          o output stream to print to
 *  @param          e error output stream
 *  @return         negative number if operation failed, otherwise the
 *                  total number of characters written
 **/
int io_printd(int32_t d, io * o, io * e)
{
  /**@todo rewrite so it does not use sprintf/fprintf**/
        NULLCHK(o, e);
        if (IO_FILE_OUT == o->type) {
                return fprintf(o->ptr.file, "%d", d);
        } else if (IO_STRING_OUT == o->type) {
                return sprintf(o->ptr.string + o->position, "%d", d);
        } else {
                /*programmer error; some kind of error reporting would be nice */
                exit(EXIT_FAILURE);
        }
        return -1;              /* returns negative like printf would on failure */
}

/**
 *  @brief          wrapper to print out a pointer; this should be rewritten
 *                  to avoid using fprintf and sprintf 
 *  @param          p pointer to print out
 *  @param          o output stream to print to
 *  @param          e error output stream
 *  @return         negative number if operation failed, otherwise the
 *                  total number of characters written
 **/
int io_printp(void *p, io * o, io * e)
{
  /**@todo rewrite so it does not use sprintf/fprintf**/
        NULLCHK(o, e);
        if (IO_FILE_OUT == o->type) {
                return fprintf(o->ptr.file, "%p", p);
        } else if (IO_STRING_OUT == o->type) {
                return sprintf(o->ptr.string + o->position, "%p", p);
        } else {
                /*programmer error; some kind of error reporting would be nice */
                exit(EXIT_FAILURE);
        }
        return -1; /* returns negative like printf would on failure */
}

/**
 *  @brief          wrapper to print out a string, *does not append newline*
 *  @param          s string to output
 *  @param          o output stream to print to
 *  @param          e error output stream
 *  @return         EOF on failure, number of characters written on success
 *                  
 **/
int io_puts(const char *s, io * o, io * e)
{
  /**@warning count can go negative when is should not!**/
        int count = 0;
        int c;
        NULLCHK(o, e);
        while ((c = *(s + (count++))))
                if (EOF == io_putc((char)c, o, e))
                        return EOF;
        return count;
}

/**
 *  @brief          A clunky function that should be rewritten for
 *                  error reporting. It is not used directly but instead
 *                  wrapped in a macro
 *  @param          s       error message to print
 *  @param          cfile   C file the error occurred in (__FILE__)
 *  @param          linenum line of C file error occurred (__LINE__)
 *  @param          e       error output stream to print to
 *  @return         void
 *                  
 **/
void io_doreport(const char *s, char *cfile, unsigned int linenum, io * e)
{
        io n_e = { IO_FILE_OUT, {NULL}, 0, 0, '\0', false };
        bool critical_failure_f = false;
        n_e.ptr.file = stderr;

        if ((NULL == e) || (NULL == e->ptr.file)
            || ((IO_FILE_OUT != e->type) && (IO_STRING_OUT != e->type))
            ) {
                e = &n_e;
                critical_failure_f = true;
        }

        io_puts("(error \"", e, e);
        io_puts(s, e, e);
        io_puts("\" \"", e, e);
        io_puts(cfile, e, e);
        io_puts("\" ", e, e);
        io_printd(linenum, e, e);
        io_puts(")\n", e, e);

        if (true == critical_failure_f) {
                io_puts("(error \"critical failure\")\n", e, e);
                exit(EXIT_FAILURE);
        }
        return;
}
