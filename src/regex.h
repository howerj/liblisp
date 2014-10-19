/**
 *  @file           regex.h
 *  @brief          regex engine, header only
 *  @author         Richard James Howe 
 *  @email          howe.r.j.89@gmail.com
 *
 *  See "regex.md" and "regex.c" for documentation.
 *
 **/
#ifndef REGEX_H
#define REGEX_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
        REGEX_FAIL_E = -1,
        REGEX_NOMATCH_E = 0,
        REGEX_MATCH_E = 1
} regex_e;

#define REGEX_MAX_DEPTH (8192u)

regex_e regex_match(char *regexp, char *text);

#ifdef __cplusplus
}
#endif
#endif
