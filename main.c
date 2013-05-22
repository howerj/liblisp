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
#include <stdio.h>
#include "lisp.h"

int main(void)
{
        file_io_t in;
        cell_t *list;
        in.fiot = io_stdin;
        in.ungetc_flag = 0;
        in.ungetc_char = '\0';

        list = parse_sexpr(&in);
        print_sexpr(list, 0);
        free_sexpr(list);
        return 0;
}
