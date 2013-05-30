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
        file_io_t in, out, err;
        cell_t *list;

        in.fiot = io_stdin;
        in.ungetc_flag = 0;
        in.ungetc_char = '\0';

        out.fiot = io_stdout;
        out.ungetc_flag = 0;
        out.ungetc_char = '\0';

        err.fiot = io_stderr;
        err.ungetc_flag = 0;
        err.ungetc_char = '\0';

        list = parse_sexpr(&in, &err);
        print_sexpr(list, 0, &out, &err);
        free_sexpr(list, &err);

        return 0;
}
