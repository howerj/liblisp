/**
 *  @file           main.c
 *  @brief          Driver for the lisp interpreter. Contains main();
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v2.0 or later version
 *  @email          howe.r.j.89@gmail.com
 *
 *  Generic TODO List:
 *
 *  @todo   Unit tests for each module
 *  @todo   Orient the lisp towards processing text à la mode de awk/sed/tr 
 *  @todo   Rewrite basic types used in implementation from arrays to cons
 *          cells as it should be.
 *
 *  @mainpage LSP Lisp
 *
 *  \section intro_sec Introduction
 *
 *  A lisp written in (mostly) ANSI C, mainly for my own education, that I hope
 *  to make into something (at least semi-) useful.
 *
 *  \section install_sec Installation
 *
 *  To make the project run:
 *
 *    make
 *
 *  At the top level. No libraries, bar the standard C library, are needed
 *  to compile the project.
 *
 **/

#include <stdio.h>              /* printf(), fopen(), fclose() */
#include <stdlib.h>             /* exit() */
#include <string.h>             /* strlen() */
#include "type.h"
#include "mem.h"
#include "sexpr.h"
#include "lisp.h"

typedef enum {
        getopt_switch,          /* 0: switch statement, eg. sets some internal bool */
        getopt_input_file,      /* 1: try to treat argument as an input file */
        getopt_output_file,     /* 2: try to redirect output to this file */
        getopt_string_input,    /* 3: lisp_eval! */
        getopt_error            /* 4: PEBKAC error: debugging is a AI complete problem */
} getopt_e;

static int getopt(char *arg);
static void setfin(io * i, FILE * in);

static bool printGlobals_f = false;
static char *usage = "./lisp -hdcpeoVG file... '(expr)'...\n";
static char *version = __DATE__ " : " __TIME__ "\n";
static char *help = "\
Program:\n\
  LSP: Lisp Interpreter\n\
Author:\n\
  Richard James Howe\n\
\n\
  -h   This message.\n\
  -d   Extra debugging information on stderr.\n\
  -c   Color on.\n\
  -p   Print out full procedure.\n\
  -e   Next argument is an expression to lisp_evaluate.\n\
  -o   Next argument is a file to write to.\n\
  -V   Version number.\n\
  -G   Print Globals list on normal exit.\n\
";

/** 
 *  @brief    Process command line options
 *  @param    arg   current command line argument
 *  @return   getopt_e, a self describing enum.        
 */
static int getopt(char *arg)
{
        int c;

        if ('-' != *arg++) {
                return getopt_input_file;
        }

        while ((c = *arg++)) {
                switch (c) {
                case 'h':
                        printf("%s%s", usage, help);
                        break;
                case 'V':
                        printf("%s", version);
                        break;
                case 'd':
                        set_mem_debug(true);
                        break;
                case 'c':
                        set_color_on(true);
                        break;
                case 'p':
                        set_print_proc(true);
                        break;
                case 'e':
                        return getopt_string_input;
                case 'o':
                        return getopt_output_file;
                case 'G':
                        printGlobals_f = true;
                        break;
                default:
                        fprintf(stderr, "unknown option: '%c'\n%s", c, usage);
                        return getopt_error;
                }
        }
        return getopt_switch;
}

/** 
 *  @brief    Set up a I/O struct to take file input
 *  @param    i     io struct to set up
 *  @param    in    Input file descriptor
 *  @return   getopt_e, a self describing enum.        
 */
static void setfin(io * i, FILE * in)
{
        memset(i, 0, sizeof(*i));
        i->type = file_in;
        i->ptr.file = in;
}

int main(int argc, char *argv[])
{
        int i;
        int nostdin = false; /*don't read from stdin after processing flags*/
        lisp l;
        FILE *input, *output;

        l = lisp_init();

        for (i = 1; i < argc; i++) {
                switch (getopt(argv[i])) {
                case getopt_switch:
                        /*getopt_switch means getopt set some flags or printed something */
                        break;
                case getopt_input_file: /* ./lisp file.lsp */
                        /*try to treat it as an output file */
                        /*printf("(input 'file \"%s\")\n", argv[i]);*/
                        if (NULL == (input = fopen(argv[i], "r"))) {
                                fprintf(stderr, "(error \"unable to read '%s'\")\n", argv[i]);
                                exit(EXIT_FAILURE);
                        }
                        setfin(l->i, input);
                        lisp_repl(l);
                        fclose(input);
                        nostdin = true;
                        break;
                case getopt_string_input: /* ./lisp -e '(+ 2 2)' */
                        if (++i < argc) {
                                memset(l->i, 0, sizeof(*l->i));
                                l->i->type = string_in;
                                l->i->ptr.string = argv[i];
                                l->i->max = strlen(argv[i]);
                                /*printf("(input 'string \"%s\")\n", argv[i]);*/
                                lisp_repl(l);
                        } else {
                                sexpr_perror(NULL, "fatal: expecting arg after -e", NULL);
                                exit(EXIT_FAILURE);
                        }
                        nostdin = true;
                        break;
                case getopt_output_file:
                        if (++i < argc) {
                                /*printf("(output 'file \"%s\")\n", argv[i]);*/
                                if (NULL == (output = fopen(argv[i], "w"))) {
                                        fprintf(stderr, "(error \"unable to write to '%s'\")\n", argv[i]);
                                        exit(EXIT_FAILURE);
                                }
                                l->o->type = file_out;
                                l->o->ptr.file = output;
                        } else {
                                sexpr_perror(NULL, "fatal: expecting arg after -o", NULL);
                                exit(EXIT_FAILURE);
                        }
                        break;
                case getopt_error:
                default:
                        sexpr_perror(NULL, "fatal: invalid command opts", NULL);
                        exit(EXIT_FAILURE);
                }
        }

        if(false == nostdin){ 
                setfin(l->i, stdin);
                lisp_repl(l);
        }

        if (true == printGlobals_f) {
                lisp_print(l->global, l->o, l->e);
        }

        lisp_end(l);

        return EXIT_SUCCESS;
}

