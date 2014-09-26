/**
 *  @file           sexpr.h
 *  @brief          General S-Expression parsing interface, header
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 **/

#ifndef SEXPR_H
#define SEXPR_H

#include "io.h"
#include "type.h"

#ifdef __cplusplus
extern "C" {
#endif

/**** macros ******************************************************************/
#define sexpr_perror(EXP,MSG,E)  dosexpr_perror((EXP),(MSG),__FILE__,__LINE__,(E))
/******************************************************************************/

/**** function prototypes *****************************************************/
void sexpr_set_color_on(bool flag);
void sexpr_set_print_proc(bool flag);
void sexpr_set_parse_numbers(bool flag);

expr sexpr_parse(io * i, io * e);
void sexpr_print(expr x, io * o, unsigned int depth, io * e);
void dosexpr_perror(expr x, char *msg, char *cfile, unsigned int linenum, io * e);
void append(expr list, expr ele, io * e);
/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif
