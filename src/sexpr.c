/**
 * Richard Howe
 *
 * License: GPL
 *
 * General s-expression parser.
 *
 * TODO:
 *  - Testing, thorough testing needs to be done.
 *  - Add handling of special types.
 *  
 */
#include "type.h"
#include "io.h"
#include "mem.h"
#include <string.h> /* strtol(), strspn(), strlen(), memset() */
#include <ctype.h> /* isspace() */

static expr parse_string(io *i, io *e);
static expr parse_symbol(io *i, io *e); /** and integers!*/
static expr parse_list(io *i, io *e);

static expr parse_symbol(io *i, io *e){ /** and integers!*/
  expr ex = NULL;
  unsigned int count = 0;
  char c, buf[BUFLEN];
  bool negative;
  ex=wcalloc(sizeof(sexpr_t), 1,e);

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
  wfree(ex,e);
  return NULL;

 success:
  ex->len = strlen(buf);
 
  /* TODO: Clean up negative handling */ 
  /** does not handle hex, or decimal points*/
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

static expr parse_string(io *i, io *e){
  expr ex = NULL;
  unsigned int count = 0;
  char c, buf[BUFLEN];

  ex = wcalloc(sizeof(sexpr_t), 1,e);
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
  wfree(ex,e);
  return NULL;

 success:
  ex->type = S_STRING;
  ex->len = strlen(buf);
  ex->data.string = wmalloc(ex->len + 1,e);
  strcpy(ex->data.string, buf);
  return ex;
}

void append(expr list, expr ele, io *e)
{ /**TODO: Error recovery, check for list type as well*/
  NULLCHK(list);
  NULLCHK(ele);
  list->data.list = wrealloc(list->data.list, sizeof(expr) * ++list->len,e);
  ((expr *) (list->data.list))[list->len - 1] = ele;
}

static expr parse_list(io *i, io *e){
  expr ex = NULL, chld;
  char c;

  ex = wcalloc(sizeof(sexpr_t), 1,e);
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
 wfree(ex,e);
 return NULL;

 success:
 ex->type = S_LIST;
 return ex;
}

expr parse_term(io *i, io *e){
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
  return NULL;
}

void print_expr(expr x, io *o, unsigned int depth, io *e){
#define indent() for(i = 0; i < depth; i++) wputc(' ',o,e)
#define emit(X)  do{ wputc((X),o,e); wputc('\n',o,e); }while(0)
  unsigned int i;
  if (!x)
    return;

  indent();
  switch (x->type) {
  case S_NIL:
    wputc('(',o,e);
    wputc(')',o,e);
    wputc('\n',o,e);
    return;
  case S_TEE:
    wprints("#t",o,e);
    return;
  case S_LIST:
    emit('(');
    for (i = 0; i < x->len; i++)
      print_expr((expr)(x->data.list[i]), o , depth + 1,e);
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
  case S_FILE:      /** fall through */
    report("UNIMPLEMENTED (TODO)");
    return;
  default:
    report("print: unassigned type");
    exit(-1);
    return;
  }
#undef indent
#undef emit
}

void free_expr(expr x, io *e){
  unsigned int i;

  if (!x)
    return;

  switch (x->type) {
  case S_TEE:
  case S_NIL:
      wfree(x,e);
      break;
  case S_LIST:
    for (i = 0; i < x->len; i++)
      free_expr((expr)((x->data.list)[i]),e);
    wfree(x->data.list,e);
    wfree(x,e);
    return;
  case S_SYMBOL:
  case S_STRING:
    wfree(x->data.string,e);
    wfree(x,e);
    return;
  case S_INTEGER:
    wfree(x,e);
    return;
  case S_PRIMITIVE:
    wfree(x,e);
    return;
  case S_FILE:      /** fall through */
    report("UNIMPLEMENTED (TODO)");
    break;
  default:
    report("free: unassigned type");
    exit(-1);
    return;
  }
}
