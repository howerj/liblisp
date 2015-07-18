/**
 *  @file           tb_regex.c
 *  @brief          Test bench for the regular expression engine
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 **/

#include "test.h"
#include "tb.h"
#include "regex.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>

void match_test(regex_e expect, char *regex, char *string, int *fail_count){
        size_t regex_len, string_len;
        char msg[256] = "fail";
        assert(regex && string && fail_count);
        regex_len = strlen(regex);
        string_len = strlen(string);
        assert(regex_len + string_len < 256u);
        sprintf(msg, "'%s' should %s match '%s'", regex, expect ? "" : "not", string);
        *fail_count += UTEST(expect != regex_match(regex,string), msg);
}


int tb_regex(void){
        int fails = 0;
#define MATCH_TEST(EXPECT,REGEX,STRING) match_test((EXPECT),(REGEX),(STRING),&fails)
        MATCH_TEST(REGEX_NOMATCH_E,"a","b");
        MATCH_TEST(REGEX_MATCH_E,"a","a");
        MATCH_TEST(REGEX_MATCH_E,"ab+c","abbc");
        return fails;
}
