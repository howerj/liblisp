/**
 *  @file           test.c
 *  @brief          The driver for the test suite, returns zero on success
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 **/
#include "test.h"
#include "tb.h"
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

struct unit_test_list{
	char *module_name;
	int (*func)(void);
	char *description;
} ulist[] = {
#define X(NAME, DESCRIPTION) {#NAME, tb_ ## NAME, DESCRIPTION},
        LIST_OF_UNITS
#undef X
	{NULL, NULL, ""}
};

int utest(int zero_is_pass, char *msg, char *file, int line){
        if(zero_is_pass || errno){
                printf("  (check-failed\n    %d \"%s\"\n    ('%s %d)",
                               zero_is_pass, msg, file, line);
                if(errno)
                        printf(" (errno %d)", errno);
                puts(")");
                errno = 0; /*clear errno for future tests*/
                return 1;
        }
        return 0;
}

int main(void){
        unsigned i, modfails = 0;
        for(i = 0; ulist[i].module_name; i++){
                int status;
                printf("(module-test\n  '%s \"%s\"\n", 
                                ulist[i].module_name, ulist[i].description);
                status = (ulist[i].func)();
                if(status){
                        modfails++;
                        fputs("  'fail", stdout);
                } else {
                        fputs("  'pass", stdout);
                }
                puts(")");
        }
        printf("(module-test-summary\n  (modules-tested %u)\n  (failure-count %u)\n  '%s)\n",
                        i, modfails, modfails?"fail":"pass");
        return modfails ? EXIT_FAILURE : EXIT_SUCCESS;
}

