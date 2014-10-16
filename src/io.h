/**
 *  @file           io.h
 *  @brief          I/O redirection and wrappers, header
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 **/

#ifndef IO_H
#define IO_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdio.h>

struct io;
typedef struct io io;

/**** macros ******************************************************************/
#define BUFLEN        (256u)
#define REPORT(X)     io_doreport((X),__FILE__,__LINE__)
/******************************************************************************/

/**** function prototypes *****************************************************/
void io_set_error_stream(io *es);
void io_string_in(io *i, char *s);
void io_string_out(io *o, char *s);
FILE *io_filename_in(io *i, char *file_name);
FILE *io_filename_out(io *o, char *file_name);
void io_file_in(io *i, FILE* file);
void io_file_out(io *o, FILE* file);
void io_file_close(io *ioc);
size_t io_sizeof_io(void);

int io_putc(char c, io * o);
int io_getc(io * i);
int io_ungetc(char c, io * i);
int io_printd(int32_t d, io * o);
int io_printp(void *p, io * o);
int io_puts(const char *s, io * o);/** error code?*/
int io_printer(io *o, char *fmt, ...);
void io_doreport(const char *s, char *cfile, unsigned int linenum);
/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif
