/**
 *  @file           lisp.h
 *  @brief          Lisp interpreter interface, header
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v2.0 or later version
 *  @email          howe.r.j.89@gmail.com
 **/

#ifndef LISP_H
#define LISP_H

lisp lisp_init(void);
lisp lisp_repl(lisp l);
void lisp_end(lisp l);

expr lisp_read(io *i, io* e);
expr lisp_eval(expr x, expr env, lisp l);
void lisp_print(expr x, io *o, io *e);
/*void lisp_doperror(expr x, io *o, io *e);*?
void lisp_clean(lisp l);

#endif
