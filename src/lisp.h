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

expr eval(expr x, expr env, lisp l);
lisp initlisp(void);
void endlisp(lisp l);

#endif
