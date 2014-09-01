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
#include "sexpr.h"

int main(void)
{
        cell_t *list;
        file_io_t in,out,err;

        in.fiot = io_stdin;
        out.fiot = io_stdout;
        err.fiot = io_stderr;

        while((list=parse_sexpr(&in,&err))!=NULL){
          print_sexpr(list,0,&out,&err);
          free_sexpr(list,&err);
        }
        return 0;
}
