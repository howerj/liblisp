/**
 *  @file           io.c
 *  @brief          I/O redirection and wrappers
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *
 *  @todo Error checking on return values.
 *
 *  This library allows redirection of input and output to
 *  various different sources. It also would allow me to add in
 *  a new arbitrary source later on, for example reading and
 *  writing to and from sockets.
 *
 *  It is possible to implement something similar using 'setbuf' and
 *  family from <stdio.h> and perhaps more cleanly and efficiently as
 *  well. However it would not *quite* replicate what I want and such
 *  functionality would have to be wrapped up anyway.
 *
 **/

#include "io.h"
#include "color.h"
#include <assert.h>  
#include <string.h>  
#include <stdlib.h>  
#include <stdarg.h>

#define NULLCHK(X)  if(NULL == (X))\
                      { REPORT("null dereference"); exit(EXIT_FAILURE);}

/**I/O abstraction structure**/
struct io {
        union {
                FILE *file;
                char *string;
        } ptr; 

        size_t position;        /* position in string */
        size_t max;             /* max string length, if known */

        enum iotype {
                IO_INVALID_E,    /* error on incorrectly set up I/O */
                IO_FILE_IN_E,    /* read from file */
                IO_FILE_OUT_E,   /* write to file */
                IO_STRING_IN_E,  /* read from a string */
                IO_STRING_OUT_E, /* write to a string, if you want */
                IO_NULL_OUT_E    /* NULL output, always such*/
        } type;

        bool ungetc;            /* true if we have ungetc'ed a character */
        char c;                 /* character store for io_ungetc() */
};

static int io_itoa(int32_t d, char *s); /* I *may* want to export this later */

static bool color_on_f = false; /*turn color on/off */
static io error_stream = {{NULL}, 0, 0, IO_FILE_OUT_E, false, '\0'};
static io *e = &error_stream; 

/**** I/O functions **********************************************************/

/**
 *  @brief          Set whether to print out colors, *does not check if
 *                  we are printing to a terminal or not, it is either on
 *                  or off*.
 *  @param          flag boolean flag to set color_on_f
 *  @return         void
 **/
void io_set_color_on(bool flag)
{
        color_on_f = flag;
}

/**
 * @brief           Set the default error reporting output stream
 * @param           es          Set the global error to this
 * @return          void
 **/
void io_set_error_stream(io *es){
        e = es;
}

/**
 * @brief           get the default error reporting output stream
 * @return          io*         The global error reporting stream
 **/
io *io_get_error_stream(void){
        return e;
}

/**
 *  @brief          Set input wrapper to read from a string
 *  @param          i           input stream, Do not pass NULL
 *  @param          s           string to read from, Do not pass NULL
 *  @return         void
 **/
void io_string_in(io *i, char *s){
        assert((NULL != i) && (NULL != s));
        memset(i, 0, sizeof(*i));
        i->type         = IO_STRING_IN_E;
        i->ptr.string   = s;
        i->max          = strlen(s);
        return;
}

/**
 *  @brief          Set output stream to point to a string
 *  @param          o           output stream, Do not pass NULL
 *  @param          s           string to write to, Do not pass NULL
 *  @param          len         maximum length of output string
 *  @return         void
 **/
void io_string_out(io *o, char *s, size_t len){
        assert((NULL != o) && (NULL != s));
        memset(o, 0, sizeof(*o));
        memset(s, '\0', len);
        o->type         = IO_STRING_OUT_E;
        o->ptr.string   = s;
        o->max          = len - 1;
        return;
}

/**
 *  @brief          Attempts to open up a file called file_name for
 *                  reading, setting the input stream wrapper to read
 *                  from it.
 *  @param          i           input stream, Do not pass NULL
 *  @param          file_name   File to open and read from, Do not pass NULL
 *  @return         FILE*       Initialized FILE*, or NULL on failure
 **/
FILE *io_filename_in(io *i, char *file_name){
        assert((NULL != i) && (NULL != file_name));
        memset(i, 0, sizeof(*i));
        i->type  = IO_FILE_IN_E;
        if(NULL == (i->ptr.file = fopen(file_name, "rb")))
                return NULL;
        return i->ptr.file;
}

/**
 *  @brief          Attempt to open file_name for writing, setting the
 *                  output wrapper to use it
 *  @param          o           output stream, Do not pass NULL
 *  @param          file_name   file to open, Do not pass NULL
 *  @return         FILE*       Initialized file pointer, or NULL on failure
 **/
FILE *io_filename_out(io *o, char *file_name){
        assert((NULL != o) && (NULL != file_name));
        memset(o, 0, sizeof(*o));
        o->type         = IO_FILE_OUT_E;
        if(NULL == (o->ptr.file = fopen(file_name, "wb")))
                return NULL;
        return o->ptr.file;
}

/**
 *  @brief          Set input stream wrapper to point to a FILE*
 *  @param          i           input stream, Do not pass NULL
 *  @param          file        file to read from, Do not pass NULL
 *  @return         void
 **/
void io_file_in(io *i, FILE* file){
        assert((NULL != i) && (NULL != file));
        memset(i, 0, sizeof(*i));
        i->type         = IO_FILE_IN_E;
        i->ptr.file     = file;
        return;
}

/**
 *  @brief          Set an output stream wrapper to use a FILE*
 *  @param          o           output stream, Do not pass NULL
 *  @param          file        FILE* to write to, Do not pass NULL
 *  @return         void
 **/
void io_file_out(io *o, FILE* file){
        assert((NULL != o) && (NULL != file));
        memset(o, 0, sizeof(*o));
        o->type         = IO_FILE_OUT_E;
        o->ptr.file     = file;
        return;
}

/**
 *  @brief          Flush and close an input or output stream, this *will not* close
 *                  stdin, stdout or stderr, but it will flush them and invalidate
 *                  the IO wrapper struct passed to it.
 *  @param          ioc         Input or output stream to close, Do not pass NULL
 *  @return         void
 **/
void io_file_close(io *ioc){
        assert(NULL != ioc);

        if((IO_FILE_IN_E == ioc->type) || (IO_FILE_OUT_E == ioc->type)){
                if(NULL != ioc->ptr.file){
                        fflush(ioc->ptr.file);
                        if((ioc->ptr.file != stdin) && (ioc->ptr.file != stdout) && (ioc->ptr.file != stdin))
                                fclose(ioc->ptr.file);
                        ioc->ptr.file = NULL;
                }
        }
        return;
}

/**
 *  @brief          Return the size of the 'io' struct, this is an incomplete
 *                  type to the outside world.
 *  @return         size_t size of io struct which is hidden from the outside
 **/
size_t io_sizeof_io(void){
        return sizeof(io);
}

/**
 *  @brief          wrapper around putc; redirect output to a file or string
 *  @param          c   output this character
 *  @param          o   output stream, Do not pass NULL
 *  @return         EOF on failure, character to output on success
 **/
int io_putc(char c, io * o)
{
        NULLCHK(o);
        NULLCHK(o->ptr.file);

        if (IO_FILE_OUT_E == o->type) {
                return fputc(c, o->ptr.file);
        } else if (IO_STRING_OUT_E == o->type) {
                if (o->position < o->max) {
                        o->ptr.string[o->position++] = c;
                        return c;
                } else {
                        return EOF;
                }
        } else if(IO_NULL_OUT_E == o->type){
                return c;
        } else {
                exit(EXIT_FAILURE); /*some kind of error reporting would be nice */
        }
        return EOF; 
}

/**
 *  @brief          wrapper around io_putc; get input from file or string
 *  @param          i input stream, Do not pass NULL
 *  @return         EOF on failure, character input on success
 **/
int io_getc(io * i)
{
        NULLCHK(i);
        NULLCHK(i->ptr.file);

        if (true == i->ungetc) {
                i->ungetc = false;
                return i->c;
        }

        if (IO_FILE_IN_E == i->type) {
                return fgetc(i->ptr.file);
        } else if (IO_STRING_IN_E == i->type) {
                return (i->ptr.string[i->position]) ? i->ptr.string[i->position++] : EOF;
        } else {
                exit(EXIT_FAILURE);/*some error message would be nice*/
        }
        return EOF;
}

/**
 *  @brief          wrapper around ungetc; unget from to file or string
 *  @param          c character to put back
 *  @param          i input stream to put character back into, Do not pass NULL
 *  @return         EOF if failed, character we put back if succeeded.
 **/
int io_ungetc(char c, io * i)
{
        NULLCHK(i);
        NULLCHK(i->ptr.file);
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
 *  @param          o output stream to print to, Do not pass NULL
 *  @return         negative number if operation failed, otherwise the
 *                  total number of characters written
 **/
int io_printd(int32_t d, io * o)
{
        char dstr[16]; /* Can hold all values of converted string */
        NULLCHK(o);
        io_itoa(d, dstr);
        return io_puts(dstr, o);
}

/**
 *  @brief          wrapper to print out a string, *does not append newline*
 *  @param          s string to output, you *CAN* pass NULL
 *  @param          o output stream to print to, Do not pass NULL
 *  @return         EOF on failure, number of characters written on success
 **/
int io_puts(const char *s, io * o)
{
        NULLCHK(o);
        if(NULL == s)
                return 0;
        if (IO_FILE_OUT_E == o->type) {
                return fputs(s,o->ptr.file);
        } else if (IO_STRING_OUT_E == o->type) {
                size_t len, newpos;
                if(o->position >= o->max)
                        return EOF;
                len = strlen(s);
                newpos = o->position + len;
                if(newpos >= o->max)
                        len = newpos - o->max;
                (void)memmove(o->ptr.string + o->position, s, len);
                o->position = newpos;
        } else if(IO_NULL_OUT_E == o->type){
                return (int)strlen(s);
        } else {
                exit(EXIT_FAILURE); /*some error message would be nice*/
        }
        return EOF;
}

/**
 * @brief       A simple printf replacement that does not handle (nor need to
 *              handle) any of the advanced formatting features that make
 *              printf...printf. It handles color formatting codes as well
 *              and some fixed width types but not floating point numbers.
 * @param       o       Write the output here
 * @param       fmt     The formatting string
 * @param       ...     Variable length number of arguments
 * @return      int     Number of character written. 
 *
 * format
 * %% -> %      %s -> string    %d -> int32_t   %c -> char
 *
 * %*(char) -> print (char) int32_t times
 *
 * If enabled and feature is compiled in print the
 * ANSI escape sequence for:
 *
 * %t -> Reset  %B -> Bold      %k -> Black     %r -> Red
 * %g -> Green  %y -> Yellow    %b -> Blue      %m -> Magenta
 * %a -> Cyan   %w -> White     %z -> Reverse V.
 *
 * Otherwise map to (nothing)
 *
 * %(default) -> (nothing)      %(EOL) -> (nothing)
 * (char) -> (char)
 *
 * It should return the number of characters written, but
 * does not at the moment.
 **/
int io_printer(io *o, char *fmt, ...)
{
        va_list ap;
        int32_t d;
        uint32_t ud;
        int count = 0;
        char c, *s;

        va_start(ap, fmt);
        while (*fmt){
                char f;
                if('%' == (f = *fmt++)){
                        switch (f = *fmt++) {
                        case '\0':/*we're done, finish up*/
                                goto FINISH;
                        case '%':
                                if(EOF != io_putc('%',o))
                                        count++;
                                break;
                        case '*':
                                if('\0' == (f = *fmt++))
                                        goto FINISH;
                                for(ud = va_arg(ap, uint32_t); ud > 0; ud--)
                                        if(EOF != io_putc(f,o))
                                                count++;
                                break;
                        case 's':      
                                s = va_arg(ap, char *);
                                count += io_puts(s,o);
                                break;
                        case 'd':      /* int */
                                d = va_arg(ap, int32_t);
                                count += io_printd(d,o);
                                break;
                        case 'c':     
                                /* need a cast here since va_arg only
                                   takes fully promoted types */
                                c = (char)va_arg(ap, int);
                                if(EOF != io_putc(c,o))
                                        count++;
                                break;
                        default:
#ifndef NO_ANSI_ESCAPE_SEQUENCES
                                if(true == color_on_f){
                                        switch(f){
                                                case 't': /*reset*/
                                                        count += io_puts(ANSI_RESET,o);
                                                        break;
                                                case 'z': /*reverse video*/
                                                        count += io_puts(ANSI_REVERSE_VIDEO,o);
                                                        break;
                                                case 'B': /*bold*/
                                                        count += io_puts(ANSI_BOLD_TXT,o);
                                                        break;
                                                case 'k': /*blacK*/
                                                        count += io_puts(ANSI_COLOR_BLACK,o);
                                                        break;
                                                case 'r': /*Red*/
                                                        count += io_puts(ANSI_COLOR_RED,o);
                                                        break;
                                                case 'g': /*Green*/
                                                        count += io_puts(ANSI_COLOR_GREEN,o);
                                                        break;
                                                case 'y': /*Yellow*/
                                                        count += io_puts(ANSI_COLOR_YELLOW,o);
                                                        break;
                                                case 'b': /*Blue*/
                                                        count += io_puts(ANSI_COLOR_BLUE,o);
                                                        break;
                                                case 'm': /*Magenta*/
                                                        count += io_puts(ANSI_COLOR_MAGENTA,o);
                                                        break;
                                                case 'a': /*cyAn*/
                                                        count += io_puts(ANSI_COLOR_CYAN,o);
                                                        break;
                                                case 'w': /*White*/
                                                        count += io_puts(ANSI_COLOR_WHITE,o);
                                                        break;
                                                default:
                                                        break;
                                        }
                                }
#endif
                                break;
                        }
                } else {
                        if(EOF != io_putc(f,o))
                                count++;
                }
        }
FINISH:
        va_end(ap);
        return count;
}

/**
 *  @brief          A clunky function that should be rewritten for
 *                  error reporting. It is not used directly but instead
 *                  wrapped in a macro
 *  @param          s       error message to print
 *  @param          cfile   C file the error occurred in (__FILE__), Do not pass NULL
 *  @param          linenum line of C file error occurred (__LINE__)
 *  @return         void
 *                  
 **/
void io_doreport(const char *s, char *cfile, unsigned int linenum)
{
        if ((NULL == e) || (NULL == e->ptr.file) || ((IO_FILE_OUT_E != e->type) && (IO_STRING_OUT_E != e->type))) {
                error_stream.ptr.file = stderr;
                error_stream.type = IO_FILE_OUT_E;
                error_stream.ungetc = false;
                io_puts("(error \"Error stream changed to default\")\n", e);
        }

        io_printer(e, "(error \"%s\" \"%s\" %d)\n", s, cfile, linenum);
        return;
}

/**** Internal functions *****************************************************/

/**
 *  @brief          Convert and integer to a string, base-10 only, the
 *                  casts are there for splint -weak checking.
 *  @param          d   integer to convert
 *  @param          s   string containing the integer, Do not pass NULL
 *  @return         int size of the converted string
 **/
static int io_itoa(int32_t d, char *s){
        uint32_t v, i;
        int8_t sign, len;
        char tb[sizeof(int32_t)*3+2]; /* maximum bytes num of new string */
        char *tbp = tb;
        assert(NULL != s);

        sign = (int32_t)(d < 0 ? -1 : 0);   
        v = (uint32_t)(sign == 0 ? d : -d);

        do{
                i = v % 10;
                v /= 10;
                *tbp++ = (char) i + ((i < 10) ? '0' : 'a' - 10);
        } while(v);

        len = tbp - tb;
        if(-1 == sign){
                *s++ = '-';
                len++;
        }

        while(tbp > tb)
                *s++ = *--tbp;
        *s = '\0';

        return (int)len;
}

