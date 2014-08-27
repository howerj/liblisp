/**
 *  @file           grep.c
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

typedef enum {
        GETOPT_SWITCH,          /* 0: switch statement, eg. sets some internal bool */
        GETOPT_REGEX,           /* 1: try to treat argument as an input file */
        GETOPT_INPUT_FILE,      /* 2: try to treat argument as an input file */
        GETOPT_ERROR            /* 3: PEBKAC error: debugging is a AI complete problem */
} getopt_e;

static getopt_e getopt(char *arg,char *progname);
static void grep(char *regex, FILE * input);
static void process_file(char *regex, char *file_name);

static bool regexset_f  = false; /* has the user provided us with a regex yet?*/
static bool verboseon_f = false; /* verbose output?*/

static char buf[BUFSZ];
static char *usage = " -hVvc regex file\n";
static char *version = __DATE__ " : " __TIME__ "\n";
static char *help = "\
Program:\n\
  grep: global/regex/print\n\
Author:\n\
  Richard James Howe\n\
\n\
  -h   This message.\n\
  -V   Version number.\n\
  -v   Make the output verbose\n\
";

/** 
 *  @brief    Process command line options
 *  @param    arg       current command line argument
 *  @param    progname  program name
 *  @return   getopt_e  a self describing enum.        
 */
static getopt_e getopt(char *arg,char *progname)
{
        int c;

        if ('-' != *arg++) {
                if(true == regexset_f){
                        return GETOPT_INPUT_FILE;
                } else {
                        regexset_f = true;
                        return GETOPT_REGEX;
                }
        }

        while ((c = *arg++)) {
                switch (c) {
                case 'h':
                        printf("%s%s%s",progname,usage, help);
                        break;
                case 'V':
                        printf("%s", version);
                        break;
                case 'v':
                        break;
                case 'c':
                        break;
                default:
                        fprintf(stderr, "unknown option: '%c'\n%s", c, usage);
                        return GETOPT_ERROR;
                }
        }
        return GETOPT_SWITCH;
}

/** 
 *  @brief    Perform search on a file
 *  @param    regex     regex to search against
 *  @param    input     inputfile
 *  @return   void
 */
static void grep(char *regex, FILE * input)
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

/** 
 *  @brief    Wrapper for grep function, ensures correct environment for it
 *  @param    regex     regex to search against
 *  @param    file_name input file name
 *  @return   void
 */
static void process_file(char *regex, char *file_name){
        FILE *input;

        if((false == regexset_f) || (NULL == regex)){
                fprintf(stderr, "regex not set.\n");
                exit(EXIT_FAILURE);
        }

        if (NULL == (input = fopen(file_name, "r"))) {
                fprintf(stderr, "Error: could not open <%s> for reading.\n", file_name);
                exit(EXIT_FAILURE);
        } else {
                printf("file:\t%s\n", file_name);
        }

        grep(regex, input);

        fflush(NULL);
        fclose(input);

        return;
}

int main(int argc, char **argv)
{
        int i;
        char *regex;
        bool processedafile_f = false;

        if (1 >= argc) {
                fprintf(stderr, "%s%s", argv[0], usage);
                exit(EXIT_FAILURE);
        }

        for(i = 1; i < argc; i++){
                switch(getopt(argv[i], argv[0])){
                        case GETOPT_SWITCH:
                                /* getopt handles this, do nothing here */
                                break;
                        case GETOPT_REGEX:
                                regex = argv[i];
                                break;
                        case GETOPT_INPUT_FILE:
                                if(true == verboseon_f)
                                        printf("file: %s\n", argv[i]);
                                process_file(regex, argv[i]);
                                processedafile_f = true;
                                break;
                        case GETOPT_ERROR: /*fall through*/
                        default:
                                fprintf(stderr,"getopt error\n");
                                return EXIT_FAILURE;
                }
        }

        if((false == processedafile_f) && (true == regexset_f))
                grep(regex, stdin);

        return EXIT_SUCCESS;
}

