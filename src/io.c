/**
 *  @file           io.c
 *  @brief          I/O redirection and wrappers
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v3.0
 *  @email          howe.r.j.89@gmail.com
 **/

#include "type.h"
#include "io.h"

/**** I/O functions **********************************************************/

/**
 *  @brief          wrapper around putc; redirect output to a file or string
 *  @param          c output this character
 *  @param          o output stream
 *  @param          e error output stream
 *  @return         EOF on failure, character to output on success
 **/
int wputc(char c, io *o, io *e){
  NULLCHK(o);
  NULLCHK(o->ptr.file);

  if(file_out == o->type){
    return fputc(c, o->ptr.file);
  } else if(string_out == o->type){
    if(o->position < o->max){
      o->ptr.string[o->position++] = c;
      return c;
    } else {
      return EOF;
    }
  } else {
    /** error */
  }
  return EOF;
}

/**
 *  @brief          wrapper around wputc; get input from file or string
 *  @param          i input stream
 *  @param          e error output stream
 *  @return         EOF on failure, character input on success
 **/
int wgetc(io *i, io *e){
  NULLCHK(i);
  NULLCHK(i->ptr.file);

  if(true == i->ungetc){
    i->ungetc = false;
    return i->c;
  }

  if(file_in == i->type){
    return fgetc(i->ptr.file);
  } else if(string_in == i->type){
    return (i->ptr.string[i->position])?i->ptr.string[i->position++]:EOF;
  } else {
    /** error */
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
int wungetc(char c, io *i, io *e){
  NULLCHK(i);
  NULLCHK(i->ptr.file);
  if(true == i->ungetc){
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
int wprintd(cell_t d, io *o, io *e){
  NULLCHK(o);
  NULLCHK(e);
  if(file_out == o->type){
    return fprintf(o->ptr.file,"%d",d);
  } else if(string_out == o->type){
    return sprintf(o->ptr.string + o->position,"%d",d);
  } else {
    /** error */
  }
  return -1; /* returns negative like printf would on failure */
}

/**
 *  @brief          wrapper to print out a string
 *  @param          s string to output
 *  @param          o output stream to print to
 *  @param          e error output stream
 *  @return         EOF on failure, number of characters written on success
 *                  
 **/
int wprints(const char *s, io *o , io *e){
  unsigned int count = 0;
  int c;
  NULLCHK(o);
  NULLCHK(e);
  while((c=*(s+(count++))))
    if (EOF == wputc((char)c,o,e))
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
void doreport(const char *s, char *cfile, unsigned int linenum, io *e)
{ /** @todo Needs rewriting so it does not use fprintf or sprintf! **/
  if((NULL == e) || (NULL == e->ptr.file)){
    fprintf(stderr, "(error\n\t(error\n\t\t\"%s\"\n\t\t\"%s\"\n\t%d\n\t)\n)\n", s, cfile, linenum);
    return;
  }

  if(file_out == e->type){
    fprintf(e->ptr.file, "(\n\terror\n\t\"%s\"\n\t\"%s\"\n\t%d\n)\n", s, cfile, linenum);
  } else if (string_out == e->type){
    sprintf(e->ptr.string,"(\n\terror\n\t\"%s\"\n\t\"%s\"\n\t%d\n)\n",s,cfile,linenum);
  } else {
    fprintf(stderr,"unknown error output stream.\n");
    exit(EXIT_FAILURE);
  }
}

