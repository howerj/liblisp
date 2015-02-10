/**
 *  @file           tb.h
 *  @brief          List of all test bench files
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 */

#ifndef TB_H
#define TB_H

#ifdef __cplusplus
extern "C" {
#endif

/*add in dependancy order*/
#define LIST_OF_UNITS\
        X(hash,         "Generic Hash library")\
        X(regex,        "Regular Expression engine")\
        X(io,           "I/O Subsystem")\
        X(mem,          "Allocator wrapper")\
        X(gc,           "Garbage collection unit")\
        X(sexpr,        "Generic S-Expression Parser")\
        X(lisp,         "Lisp Expression Evaluator")\
        X(linenoise,    "Line editing library")

/*function prototype definition for all test units*/
#define X(FUNCTION, DESCRIPTION) int tb_ ## FUNCTION (void);
        LIST_OF_UNITS
#undef X

#ifdef __cplusplus
}
#endif
#endif
