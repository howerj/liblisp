/**
 *  @file           io.h
 *  @brief          I/O redirection and wrappers, header
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v2.0 or later version
 *  @email          howe.r.j.89@gmail.com
 **/

#ifndef IO_H
#define IO_H

/**** macros ******************************************************************/
#define BUFLEN        (256u)
#define report(X,E)   doreport((X),__FILE__,__LINE__,(E))
#define NULLCHK(X,E)  if(NULL == (X))\
                      { report("null dereference",(E)); exit(EXIT_FAILURE);}
/******************************************************************************/

/**** function prototypes *****************************************************/
int io_putc(char c, io * o, io * e);
int io_getc(io * i, io * e);
int io_ungetc(char c, io * i, io * e);
int io_printd(cell_t d, io * o, io * e);
int io_printp(void *p, io * o, io * e);
int io_puts(const char *s, io * o, io * e);/** error code?*/
void doreport(const char *s, char *cfile, unsigned int linenum, io * e);
/******************************************************************************/
#endif
