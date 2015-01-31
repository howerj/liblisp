/**
 *  @file           main.c
 *  @brief          Driver for the lisp interpreter. Contains main();
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
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

#include "lisp.h"
#include "mem.h"
#include "sexpr.h"

typedef enum {
        getopt_switch,          /* 0: switch statement, eg. sets some internal bool */
        getopt_input_file,      /* 1: try to treat argument as an input file */
        getopt_output_file,     /* 2: try to redirect output to this file */
        getopt_string_input,    /* 3: lisp_eval! */
        getopt_error            /* 4: PEBKAC error: debugging is a AI complete problem */
} getopt_e;

static int getopt(char *arg);

static bool main_debug_f = false;
static char *usage = "./lisp -hdcpeoVG file... '(expr)'...\n";
static char *version = __DATE__ " : " __TIME__ "\n";
static char *help = "\
Program:\n\
  LSP: Lisp Interpreter\n\
Author:\n\
  Richard James Howe\n\
\n\
  -h   This message.\n\
  -d   Extra debugging information.\n\
  -c   Color on.\n\
  -p   Print out full procedure.\n\
  -e   Next argument is an expression to lisp_evaluate.\n\
  -o   Next argument is a file to write to.\n\
  -V   Version number.\n\
";

/** 
 *  @brief    Process command line options
 *  @param    arg   current command line argument
 *  @return   getopt_e, a self describing enum.        
 */
static int getopt(char *arg)
{
        int c;
        io *e = io_get_error_stream();

        if ('-' != *arg++) {
                return getopt_input_file;
        }

        while ((c = *arg++)) {
                switch (c) {
                case 'h':
                        io_printer(e, "%s%s", usage, help);
                        break;
                case 'V':
                        io_puts(version, e);
                        break;
                case 'd':
                        mem_set_debug(true);
                        main_debug_f = true;
                        break;
                case 'c':
                        io_set_color_on(true);
                        break;
                case 'p':
                        sexpr_set_print_proc(true);
                        break;
                case 'e':
                        return getopt_string_input;
                case 'o':
                        return getopt_output_file;
                default:
                        io_printer(e, "unknown option: '%c'\n%s", c, usage);
                        return getopt_error;
                }
        }
        return getopt_switch;
}

int main(int argc, char *argv[])
{
        int i;
        bool nostdin_f = false; /*don't read from stdin after processing flags */
        lisp l;

        l = lisp_init();

        for (i = 1; i < argc; i++) {
                switch (getopt(argv[i])) {
                case getopt_switch:
                        /*getopt_switch means getopt set some flags or printed something */
                        break;
                case getopt_input_file: /* ./lisp file.lsp */
                        /*try to treat it as an output file */
                        if(main_debug_f) 
                                io_printer(l->o,"(input 'file %s)\n",argv[i]);
                        if(NULL == io_filename_in(l->i, argv[i])){
                                SEXPR_PERROR(NULL, "fatal: could not open file for reading");
                                exit(EXIT_FAILURE);
                        }
                        /** @todo lisp_repl(l) should return an error expr on failure **/
                        (void)lisp_repl(l); 
                        io_file_close(l->i);
                        nostdin_f = true;
                        break;
                case getopt_string_input:  /* ./lisp -e '(+ 2 2)' */
                        if (++i < argc) {
                                if(main_debug_f) 
                                        io_printer(l->o,"(input 'string %s)\n",argv[i]);
                                io_string_in(l->i, argv[i]);
                                /** @todo lisp_repl(l) should return an error expr on failure **/
                                (void)lisp_repl(l);
                        } else {
                                SEXPR_PERROR(NULL, "fatal: expecting arg after -e");
                                exit(EXIT_FAILURE);
                        }
                        nostdin_f = true;
                        break;
                case getopt_output_file:
                        if (++i < argc) {
                                if(main_debug_f) 
                                        io_printer(l->o,"(output 'file %s)\n",argv[i]);
                                if(NULL == io_filename_out(l->o,argv[i])){
                                        SEXPR_PERROR(NULL, "fatal: could not open file for writing");
                                        exit(EXIT_FAILURE);
                                }
                        } else {
                                SEXPR_PERROR(NULL, "fatal: expecting arg after -o");
                                exit(EXIT_FAILURE);
                        }
                        break;
                case getopt_error:
                default:
                        SEXPR_PERROR(NULL, "fatal: invalid command opts");
                        exit(EXIT_FAILURE);
                }
        }

        if (false == nostdin_f) {
                io_file_in(l->i, stdin);
                /** @todo lisp_repl(l) should return an error expr on failure **/
                (void)lisp_repl(l);
        }

        if (true == main_debug_f)
                lisp_print(l->global_head, l->o);

        lisp_end(l);

        return EXIT_SUCCESS;
}

