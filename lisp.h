/* 
 * Richard James Howe
 * Howe Lisp.
 *
 * Lisp interpreter.
 *
 * @author         Richard James Howe.
 * @copyright      Copyright 2013 Richard James Howe.
 * @license        LGPL      
 * @email          howe.r.j.89@gmail.com
 */

#ifndef lisp_header_guard
#define lisp_header_guard

#define true    1
#define false   0

#define MAX_STR 256

enum error_type{
  err_ok,
  err_generic_parse
};

enum cell_type{
  type_null,
  type_list,
  type_int,
  type_symbol,
  type_str,
  type_error
};

union cell_content{
  int i;
  struct cell *cell;
  char *s;
};

struct cell{
  enum cell_type type;
  union cell_content car;
  union cell_content cdr;
};

typedef struct cell cell_t;

#endif
