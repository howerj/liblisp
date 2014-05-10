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
int wputc(char c, io *p, io *e){
  NULLCHK(p);
  NULLCHK(p->ptr.file);

  if(file_out == p->type){
    return fputc(c, p->ptr.file);
  } else if(string_out == p->type){
    if(p->position < p->max)
      p->ptr.string[p->position++] = c;
    else
      return EOF;
  } else {
    /** error */
  }
  return EOF;
}

int wgetc(io *p, io *e){
  NULLCHK(p);
  NULLCHK(p->ptr.file);

  if(true == p->ungetc){
    p->ungetc = false;
    return p->c;
  }

  if(file_in == p->type){
    return fgetc(p->ptr.file);
  } else if(string_in == p->type){
    return (p->ptr.string[p->position])?p->ptr.string[p->position++]:EOF;
  } else {
    /** error */
  }
  return EOF;
}

int wungetc(char c, io *p, io *e){
  NULLCHK(p);
  NULLCHK(p->ptr.file);
  p->c = c;
  p->ungetc = true;
  return c;
}

int wprintd(cell_t d, io *o, io *e){
  NULLCHK(o);
  NULLCHK(e);
  if(file_out == o->type){
    return fprintf(o->ptr.file,"%d",d);
  } else if(string_out == o->type){
    return sprintf(o->ptr.string,"%d",d);
  } else {
    /** error */
  }
  return 1;
}

void wprints(const char *s, io *o , io *e){
  int c;
  NULLCHK(o);
  NULLCHK(e);
  while((c=*s++))
    wputc((char)c,o,e);
}

void doreport(const char *s, char *cfile, unsigned int linenum, io *e)
{ /** TODO: do report needs rewriting so it does not use fprintf or sprintf!*/
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
    exit(-1);
  }
}

