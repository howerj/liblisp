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
expr lisp_eval(expr x, expr env, lisp l);
int  lisp_repl(lisp l);
void lisp_end(lisp l);

#endif
