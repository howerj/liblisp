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
#include "lisp.h"
#include <stdio.h>
/*
static char   wrap_get();
static int    wrap_put();
*/
static char   *parse_string(FILE *input);
static char   *parse_symbol(FILE *input);
static int     parse_int(FILE *input);
static cell_t *parse_list(FILE *input);
cell_t        *parse_sexpr(FILE *input);
void           print_sexpr(cell_t *cell);


int     lisp_interpret(cell_t *cell);
int     lisp_monitor(cell_t *cell);
