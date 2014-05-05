/** modified from 
 * <http://rosettacode.org/wiki/S-Expressions#C>
 *
 *  @brief This file handles s-expression parsing,
 *  error reporting and most I/O related functions.
 *
 *  TODO:
 *    * Better error checking
 *    * Add S_INTEGER parsable type
 *    * and S_FUNCTION, S_FILE, S_POINTER invisible types
 *    * Maximum depth!
 *    * Fatal errors? exit(-1) or better handlers?
 *    * Don't currently like error handling atm...
 *    * realloc test
 *    * Needs to handle EOF correctly.
 *
 *  NOTE:
 *    * x = expression
 *    * i = input
 *    * o = output
 *    * e = error
 */

/**** includes ***************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include "sexpr.h"
/*****************************************************************************/

/**** macros *****************************************************************/
#define CALLOC_OR_FAIL()\
  if(!(ex=wcalloc(sizeof(sexpr_t), 1,e))){\
    report("calloc() failed");\
    goto fail;\
  }
/*****************************************************************************/

/*****************************************************************************/

/**** function prototypes (local) ********************************************/
static int wgetc(io *p, io *e);
static int wputc(char c, io *p, io *e);
static int wungetc(char c, io *p, io *e);
static int wprintd(int d, io *o, io *e);

static expr parse_string(io *i, io *e);
static expr parse_symbol(io *i, io *e);
static expr parse_list(io *i, io *e);
/*****************************************************************************/

/**** I/O functions **********************************************************/
static int wputc(char c, io *p, io *e){
  NULLCHK(p);
  NULLCHK(p->ptr.file);

  if(file_out == p->type){
    return fputc(c, p->ptr.file);
  } else if(string_out == p->type){
    if(p->position < p->max)
      p->ptr.string[p->position++] = c;
    else
      return EOF;
    /** else error */
  } else {
    /** error */
  }
  return EOF;
}

static int wgetc(io *p, io *e){
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

static int wungetc(char c, io *p, io *e){
  NULLCHK(p);
  NULLCHK(p->ptr.file);
  p->c = c;
  p->ungetc = true;
  return c;
}

static int wprintd(int d, io *o, io *e){
  NULLCHK(o);
  NULLCHK(e);
  if(file_out == o->type){
    return fprintf(o->ptr.file,"%d",d);
  } else if(string_out == o->type){
    return sprintf(o->ptr.string,"%d",d);
  } else {
    /** error */
  }
  return -1;
}

void doreport(const char *s, char *cfile, unsigned int linenum, io *e)
{
  if((NULL == e) || (NULL == e->ptr.file)){
    fprintf(stderr, "(error\n\t(error\n\t\t\"%s\"\n\t\t\"%s\"\n\t%d\n\t)\n)\n", s, cfile, linenum);
    return;
  }

  if(file_out == e->type){
    fprintf(e->ptr.file, "(error\n\t\"%s\"\n\t\"%s\"\n\t%d\n)\n", s, cfile, linenum);
  } else if (string_out == e->type){
    sprintf(e->ptr.string,"(error\n\t\"%s\"\n\t\"%s\"\n\t%d\n)\n",s,cfile,linenum);
  } else {

  }
}
/*****************************************************************************/

/**** malloc wrappers ********************************************************/

void *wmalloc(size_t size, io *e){
  void* v;
  if(!e){ /** io might not be set up yet */
  }
  v = malloc(size);
  return v;
}

void *wcalloc(size_t num, size_t size, io *e){
  void* v;
  if(!e){ /** io might not be set up yet */
  }
  v = calloc(num,size);
  return v;
}

void *wrealloc(void *ptr, size_t size, io *e){
  /** should zero the extra allocated bytes */
  void* v;
  if(!e){ /** io might not be set up yet */
  }
  v = realloc(ptr,size);
  return v;
}

void wfree(void *ptr, io *e){
  if(!e){ /** io might not be set up yet */
  }
  free(ptr);
}

/*****************************************************************************/


/**** parsing functions ******************************************************/
static expr parse_string(io *i, io *e)
{
  expr ex = NULL;
  unsigned int count = 0;
  char c, buf[BUFLEN];

  memset(buf, '\0', BUFLEN);
  CALLOC_OR_FAIL();

  while (EOF!=(c=wgetc(i,e))){
    if (BUFLEN <= count) {
      report("string too long");
      /*report(s);*/
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
        /*report(s);*/
        goto fail;
      }
    case '"':
      goto success;
    default:
      buf[count++] = c;
    }
  }
 fail:
  wfree(ex,e);
  return 0;

 success:
  ex->type = S_STRING;
  ex->len = strlen(buf);
  if (!(ex->buf = wmalloc(ex->len + 1,e)))
    goto fail;
  strcpy(ex->buf, buf);
  return ex;
}

static expr parse_symbol(io *i, io *e) /** and integers!*/
{
  expr ex = NULL;
  unsigned int count = 0;
  char c, *b, buf[BUFLEN];

  memset(buf, '\0', BUFLEN);
  CALLOC_OR_FAIL();

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
  wfree(ex,e);
  return 0;

 success:
  ex->len = strlen(buf);
  
  b=(('-'==buf[0])||('+'==buf[0]))?buf+1:buf;

  /** does not handle hex, or decimal points*/
  if(strspn(b,"0123456789")==ex->len){ 
    ex->type = S_INTEGER;
    ex->buf = (void*)strtol(buf,NULL,0);
  } else{
    ex->type = S_SYMBOL;
    if (!(ex->buf = wmalloc(ex->len + 1,e)))
      goto fail;
    strcpy(ex->buf, buf);
  }
  return ex;
}

void append(expr list, expr ele, io *e)
{
  NULLCHK(list);
  NULLCHK(ele);
  list->buf = wrealloc(list->buf, sizeof(expr) * ++list->len,e);
  ((expr *) (list->buf))[list->len - 1] = ele;
}

static expr parse_list(io *i, io *e)
{
  expr ex = NULL, chld;
  char c;

  CALLOC_OR_FAIL();
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
      append(ex, chld,e);
      continue;
    case '(':
      chld = parse_list(i,e);
      if (!chld)
        goto fail;
      append(ex, chld,e);
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
  /*report(s);*/
  wfree(ex,e);
  return 0;

 success:
  ex->type = S_LIST;
  return ex;
}

expr parse_term(io *i,io *e)
{
  char c;
  while (EOF!=(c=wgetc(i,e))){
    if (isspace(c)) {
      continue;
    }
    switch (c) {
    case '(':
      return parse_list(i,e);
    case '"':
      return parse_string(i,e);
    default:
      wungetc(c,i,e);
      return parse_symbol(i,e);
    }
  }
  return 0;
}
/*****************************************************************************/

/**** expression printing ****************************************************/
void print_expr(expr x, io *o, unsigned int depth, io *e)
{
#define indent() for(i = 0; i < depth; i++) wputc(' ',o,e)
#define emit(X)  do{ wputc((X),o,e); wputc('\n',o,e); }while(0)
  unsigned int i;
  if (!x)
    return;

  switch (x->type) {
  case S_NONE:
    report("unassigned type");
    break;
  case S_LIST:
    indent();
    emit('(');
    for (i = 0; i < x->len; i++)
      print_expr(((expr *) x->buf)[i], o , depth + 1,e);
    indent();
    emit(')');
    return;
  case S_SYMBOL:
  case S_STRING:
    indent();
    if (x->type == S_STRING)
      wputc('"', o,e);
    for (i = 0; i < x->len; i++) {
      switch (((char *)x->buf)[i]) {
      case '"':
      case '\\':
        wputc('\\', o,e);
        break;
      case ')':
      case '(':
        if (x->type == S_SYMBOL)
          wputc('\\',o,e);
      }

      wputc(((char *)x->buf)[i], o,e);
    }
    if (x->type == S_STRING)
      wputc('"', o,e);
    wputc('\n', o,e);
    return;
  case S_INTEGER:
    indent();
    wprintd((int)x->buf, o, e);
    wputc('\n', o,e);
    return;
  case S_PRIMITIVE: /** fall through */
  case S_FILE:      /** fall through */
  case S_FUNCTION:  /** fall through */
  case S_POINTER:   /** catch!*/
    report("UNIMPLEMENTED (TODO)");
    break;
  default:
    report("unassigned type");
    exit(-1);
    return;
  }
#undef indent
#undef emit
}
/*****************************************************************************/

/**** freeing expressions ****************************************************/
void free_expr(expr x, io *e)
{
  unsigned int i;

  if (!x)
    return;

  switch (x->type) {
  case S_NONE:
      report("unassigned type");
      break;
  case S_LIST:
    for (i = 0; i < x->len; i++)
      free_expr(((expr *) x->buf)[i],e);
    wfree(x->buf,e);
    wfree(x,e);
    return;
  case S_SYMBOL:
  case S_STRING:
    wfree(x->buf,e);
    wfree(x,e);
    return;
  case S_INTEGER:
    wfree(x,e);
    return;
  case S_PRIMITIVE: /** fall through */
  case S_FILE:      /** fall through */
  case S_FUNCTION:  /** fall through */
  case S_POINTER:   /** catch!*/
    report("UNIMPLEMENTED (TODO)");
    break;
  default:
    report("unassigned type");
    exit(-1);
    return;
  }
}
/*****************************************************************************/

#undef CALLOC_OR_FAIL
