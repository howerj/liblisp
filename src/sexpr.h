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

#ifdef __cplusplus
extern "C" {
#endif

#include "io.h"
#include "type.h"

/**** macros ******************************************************************/
#define SEXPR_PERROR(EXP,MSG)  dosexpr_perror((EXP),(MSG),__FILE__,__LINE__)
/******************************************************************************/

/**** function prototypes *****************************************************/
void sexpr_set_print_proc(bool flag);
void sexpr_set_parse_numbers(bool flag);

expr sexpr_parse(io * i);
void sexpr_print(expr x, io * o, unsigned int depth);
void dosexpr_perror(expr x, char *msg, char *cfile, unsigned int linenum);
void append(expr list, expr ele);
/******************************************************************************/
#ifdef __cplusplus
}
#endif
#endif
