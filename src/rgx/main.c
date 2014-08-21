/**
 *  @file           main.c
 *  @brief          Test bench for regex engine, implements grep.
 *  @author         Richard James Howe 
 *  @email          howe.r.j.89@gmail.com
 *
 *  See "regex.md" and "regex.h" for documentation.
 *
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "regex.h"

#define BUFSZ (4096u)
char buf[BUFSZ];

void grep(char *regex, FILE * input)
{
        unsigned int i = 0;
        memset(buf, '\0', BUFSZ);
        while (fgets(buf, BUFSZ, input)) {
                i++;
                buf[strlen(buf) - 1] = '\0';    /*replace newline */
                if (REGEX_MATCH == regex_match(regex, buf)) {
                        printf("%s\n", buf);
                }
                memset(buf, '\0', BUFSZ);
        }
}

int main(int argc, char **argv)
{
        FILE *input;
        unsigned int i;

        if (1 >= argc) {
                fprintf(stderr, "usage: %s <regex> <file>\n", argv[0]);
                exit(EXIT_FAILURE);
        }

        printf("regex:%s\n", argv[1]);
        if (2 == argc) {
                puts("file:\tstdin");
                input = stdin;

                grep(argv[1], input);

                fflush(NULL);
        } else {
                for (i = 2; i < argc; i++) {
                        if (NULL == (input = fopen(argv[i], "r"))) {
                                fprintf(stderr, "Error: could not open <%s> for reading.\n", argv[i]);
                                exit(EXIT_FAILURE);
                        } else {
                                printf("file:\t%s\n", argv[i]);
                        }

                        grep(argv[1], input);

                        fflush(NULL);
                        fclose(input);

                }
        }
        return EXIT_SUCCESS;
}
