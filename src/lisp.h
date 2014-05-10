/**
 *  @file           lisp.h
 *  @brief          Lisp interpreter interface, header
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v3.0
 *  @email          howe.r.j.89@gmail.com
 **/

#ifndef LISP_H
#define LISP_H

expr eval(expr x, expr env, lisp l);
lisp initlisp(void);

#endif
