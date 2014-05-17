/**
 *  @file           sexpr.c
 *  @brief          General S-Expression parser
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v3.0
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 *  This S-expression parser, although made for a lisp interpreter,
 *  is a small, generic, parser that could be used for other projects.
 *
 *  The two main functions 
 *    - Parse an S-expression (parse_term)
 *    - Print out an S-expression (print_expr)
 *
 *  There are sub-functions called by the parser that could be useful
 *  in their own right and might change so they can be accessed externally
 *  later.
 *
 *  There are a number of types that are defined in "types.h" that it
 *  *could* return but does not.
 *
 **/

#include "type.h"   /* Project wide types */
#include "io.h"     /* I/O wrappers */
#include "mem.h"    /* free wrappers */
#include <string.h> /* strtol(), strspn(), strlen(), memset() */
#include <ctype.h>  /* isspace() */

static expr parse_string(io *i, io *e);
static expr parse_symbol(io *i, io *e); /* and integers!*/
static expr parse_list(io *i, io *e);

/*** interface functions *****************************************************/

/**
 *  @brief          Parses a list or atom, returning the root
 *                  of the S-expression.
 *  @param          i input stream
 *  @param          e error output stream
 *  @return         NULL or parsed list
 **/
expr parse_term(io *i, io *e){
  char c;
  while (EOF!=(c=wgetc(i,e))){
    if (isspace(c)) {
      continue;
    }
    switch (c) {
    case ')':
      report("unmatched ')'");
      return NULL;
    case '(':
      return parse_list(i,e);
    case '"':
      return parse_string(i,e);
    default:
      wungetc(c,i,e);
      return parse_symbol(i,e);
    }
  }
  return NULL;
}

/**
 *  @brief          Recursively prints out an s-expression
 *  @param          x     expression to print
 *  @param          o     output stream
 *  @param          depth current depth of expression
 *  @param          e     error output stream
 *  @return         void
 **/
void print_expr(expr x, io *o, unsigned int depth, io *e){
#define indent() for(i = 0; i < depth; i++) wputc(' ',o,e)
#define emit(X)  do{ wputc((X),o,e); wputc('\n',o,e); }while(0)
  unsigned int i;
  if (!x)
    return;

  indent();
  switch (x->type) {
  case S_NIL:
    wprints("()\n",o,e);
    return;
  case S_TEE:
    wprints("#t\n",o,e);
    return;
  case S_LIST:
    emit('(');
    for (i = 0; i < x->len; i++)
      print_expr(x->data.list[i], o , depth + 1,e);
    indent();
    emit(')');
    return;
  case S_SYMBOL:
  case S_STRING:
    if (x->type == S_STRING)
      wputc('"', o,e);
    for (i = 0; i < x->len; i++) {
      switch ((x->data.string)[i]) {
      case '"':
      case '\\':
        wputc('\\', o,e);
        break;
      case ')':
      case '(':
        if (x->type == S_SYMBOL)
          wputc('\\',o,e);
      }
      wputc((x->data.string)[i], o,e);
    }
    if (x->type == S_STRING)
      wputc('"',o,e);
    wputc('\n',o,e);
    return;
  case S_INTEGER:
    wprintd(x->data.integer,o,e);
    wputc('\n',o,e);
    return;
  case S_PRIMITIVE:
    wprints("#PRIMOP\n",o,e);
    return;
  case S_PROC: 
    wprints("#PROC\n",o,e); /** @todo print out procedure?**/
    return;
  case S_FILE: /** @todo implement file support, then printing**/     
    report("UNIMPLEMENTED (TODO)");
    return;
  default: /* should never get here */
    report("print: not a known printable type");
    exit(EXIT_FAILURE);
    return;
  }
#undef indent
#undef emit
}

/**
 *  @brief          Print out an error message while trying to handle
 *                  invalid arguments gracefully. A macro called
 *                  print_error should be defined in the header so you
 *                  do not have to pass cfile and linenum constantly to it.
 *  @param          x       NULL or expression to print
 *  @param          msg     Error message to print
 *  @param          cfile   File error occurred in 
 *  @param          linenum Line number error occurred on
 *  @param          e       Output wrapper stream, if NULL, default to stderr
 *  @return         void
 **/
void doprint_error(expr x, char *msg, char *cfile, unsigned int linenum, io *e){
  static io fallback = { file_out, { NULL }, 0, 0, 0, false };
  fallback.ptr.file = stderr;

  if((NULL == e) || (NULL == e->ptr.file))
    e = &fallback;

  wprints("(error \"",e,e); 
  wprints(msg,e,e); 
  wprints("\" \"",e,e); 
  wprints(cfile,e,e); 
  wprints("\" ",e,e); 
  wprintd(linenum,e,e);
  if(NULL == x){
  } else {
    wputc('\n',e,e);
    print_expr(x,e,1,e);
  }
  wprints(")\n",e,e); 
  return;
}

/*****************************************************************************/

/**
 *  @brief          parse a symbol or integer (in decimal or
 *                  octal format, positive or negative)
 *  @param          i input stream
 *  @param          e error output stream
 *  @return         NULL or parsed symbol / integer
 **/
static expr parse_symbol(io *i, io *e){ /* and integers!*/
  expr ex = NULL;
  unsigned int count = 0;
  char c, buf[BUFLEN];
  bool negative;
  ex = gccalloc(e);

  memset(buf, '\0', BUFLEN);

  while (EOF!=(c=wgetc(i,e))){
    if (BUFLEN <= count) {
      report("symbol too long");
      report(buf);
      goto fail;
    }
    if (isspace(c))
      goto success;
    if ((c == ')') || (c == '(')) {
      wungetc(c,i,e);
      goto success;
    }

    switch (c) {
    case '\\':
      switch (c=wgetc(i,e)) {
      case '\\':
      case '"':
      case '(':
      case ')':
        buf[count++] = c;
        continue;
      default:
        report(buf);
        goto fail;
      }
    case '"':
      report(buf);
      goto success;
    default:
      buf[count++] = c;
    }
  }

 fail:
  return NULL;

 success:
  ex->len = strlen(buf);

  /** @todo Clean up negative handling and handle hex **/
  negative=(('-'==buf[0])||('+'==buf[0]))&&(ex->len-1)?true:false;
  if(strspn(negative?buf+1:buf,"0123456789")==(ex->len-(negative?1:0))){
    ex->type = S_INTEGER;
    ex->data.integer = strtol(buf,NULL,0);
  } else{
    ex->type = S_SYMBOL;
    ex->data.symbol = wmalloc(ex->len + 1,e);
    strcpy(ex->data.symbol, buf);
  }
  return ex;
}

/**
 *  @brief          parse a string into a s-expr atom
 *  @param          i input stream
 *  @param          e error output stream
 *  @return         NULL or parsed string
 **/
static expr parse_string(io *i, io *e){
  expr ex = NULL;
  unsigned int count = 0;
  char c, buf[BUFLEN];

  ex = gccalloc(e);
  memset(buf, '\0', BUFLEN);

  while (EOF!=(c=wgetc(i,e))){
    if (BUFLEN <= count) {
      report("string too long");
      report(buf); /* check if correct */
      goto fail;
    }
    switch (c) {
    case '\\':
      switch (c=wgetc(i,e)) {
      case '\\':
      case '"':
        buf[count++] = c;
        continue;
      default:
        report("invalid escape char");
        report(buf);
        goto fail;
      }
    case '"':
      goto success;
    default:
      buf[count++] = c;
    }
  }
 fail:
  return NULL;

 success:
  ex->type = S_STRING;
  ex->len = strlen(buf);
  ex->data.string = wmalloc(ex->len + 1,e);
  strcpy(ex->data.string, buf);
  return ex;
}

/**
 *  @brief          Takes an already existing list and appends an atom to it
 *  @param          list a list to append an atom to
 *  @param          ele  the atom to append to the list
 *  @param          e    error output stream
 *  @return         void
 *
 *  @todo           Error handline
 *  @todo           Check for list type OR proc type
 **/
void append(expr list, expr ele, io *e)
{ 
  NULLCHK(list);
  NULLCHK(ele);
  list->data.list = wrealloc(list->data.list, sizeof(expr) * ++list->len,e);
  (list->data.list)[list->len - 1] = ele;
}

/**
 *  @brief          Parses a list, consisting of strings, symbols,
 *                  integers, or other lists.
 *  @param          i input stream
 *  @param          e error output stream
 *  @return         NULL or parsed list
 **/
static expr parse_list(io *i, io *e){
  expr ex = NULL, chld;
  char c;

  ex = gccalloc(e);
  ex->len = 0;

  while (EOF!=(c=wgetc(i,e))){
    if (isspace(c)) {
      continue;
    }

    switch (c) {
    case '"':
      chld = parse_string(i,e);
      if (!chld)
        goto fail;
      append(ex,chld,e);
      continue;
    case '(':
      chld = parse_list(i,e);
      if (!chld)
        goto fail;
      append(ex,chld,e);
      continue;
    case ')':
      goto success;

    default:
      wungetc(c,i,e);
      chld = parse_symbol(i,e);
      if (!chld)
        goto fail;
      append(ex,chld,e);
      continue;
    }
  }

 fail:
 report("list err");
 return NULL;

 success:
 ex->type = S_LIST;
 return ex;
}


