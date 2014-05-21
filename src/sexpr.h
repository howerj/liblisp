/**
 *  @file           sexpr.h
 *  @brief          General S-Expression parsing interface, header
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v2.0 or later version
 *  @email          howe.r.j.89@gmail.com
 **/

#ifndef SEXPR_H
#define SEXPR_H

/**** macros ******************************************************************/
#define print_error(EXP,MSG,E)  doprint_error((EXP),(MSG),__FILE__,__LINE__,(E))
/******************************************************************************/

/**** function prototypes *****************************************************/
expr parse_term(io *i, io *e);
void print_expr(expr x, io *o, unsigned int depth, io *e);
void doprint_error(expr x, char *msg, char *cfile, unsigned int linenum, io *e);
void append(expr list, expr ele, io *e);
/******************************************************************************/

#endif
