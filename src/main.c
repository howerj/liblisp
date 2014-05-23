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
 *
 *  @mainpage LSP (Lispy Space Princess) Lisp
 *
 *  \section intro_sec Introduction
 *
 *  A lisp written in ANSI C, mainly for my own education, that I hope
 *  to make into something (at least semi-) useful.
 *
 *  \section install_sec Installation
 *
 *  To make the project run:
 *
 *    make
 *
 *  At the top level. No libraries bar the standard C library is needed
 *  to compile the project.
 *
 **/

#include <stdio.h>  /* printf(), fopen(), fclose() */
#include <stdlib.h> /* exit() */
#include <string.h> /* strlen() */
#include "type.h"
#include "mem.h"
#include "sexpr.h"
#include "lisp.h"

typedef enum{
  getopt_switch,        /* 0: switch statement, eg. sets some internal bool */
  getopt_input_file,    /* 1: try to treat argument as an input file */
  getopt_output_file,   /* 2: try to redirect output to this file */
  getopt_string_input,  /* 3: eval! */
  getopt_error          /* 4: PEBKAC error: debugging is a AI complete problem*/
} getopt_e;

static int getopt(char *arg);
static int repl(lisp l);

static bool printGlobals_f = false;
static char *usage = "./lisp -hdcpeoVG file... '(expr)'...\n";

/**
 * version should include md5sum calculated from
 *  c and h files, excluding the file it gets put
 *  into. This will be included here.
 *
 *  Generation would be like this:
 *  md5sum *.c *.h | md5sum | more_processing > version/version.h
 *  in the makefile
 */
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
  -e   Next argument is an expression to evaluate.\n\
  -o   Next argument is a file to write to.\n\
  -V   Version number.\n\
  -G   Print Globals list on normal exit.\n\
";

/** 
 *  @brief    Process command line options
 *  @param    arg   current command line argument
 *  @return   getopt_e, a self describing enum.        
 */
static int getopt(char *arg){
  int c;

  if('-' != *arg++){ 
    return getopt_input_file;
  }

  while((c = *arg++)){
    switch(c){
      case 'h':
        printf("%s",usage);
        printf("%s",help);
        break;
      case 'V':
        printf("%s",version);
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
        fprintf(stderr,"unknown option: '%c'\n%s", c, usage);
        return getopt_error;
    }
  }
  return getopt_switch;
}

/** 
 *  @brief    repl implements a lisp Read-Evaluate-Print-Loop
 *  @param    l an initialized lisp environment
 *  @return   Always zero at the moment
 *
 *  @todo When Error Expression have been properly implemented any
 *        errors that have not been caught should be returned by repl
 *        or handled by it to avoid multiple error messages being printed
 *        out.
 */
static int repl(lisp l){
  expr x;
  while(NULL != (x = parse_term(l->i, l->e))){
    x = eval(x,l->env,l);
    print_expr(x,l->o,0,l->e);
    gcmark(l->global,l->e);
    gcsweep(l->e);
  }
  return 0;
}

int main(int argc, char *argv[]){
  int i;
  lisp l;
  FILE *input, *output;

  l = initlisp();

  for(i = 1; i < argc; i++){
    switch(getopt(argv[i])){
      case getopt_switch:
        /*getopt_switch means getopt set some flags or printed something*/
        break;
      case getopt_input_file:
        /*try to treat it as an output file*/
        printf("(input 'file \"%s\")\n",argv[i]);
        if(NULL == (input = fopen(argv[i],"r"))){
          fprintf(stderr,"(error \"unable to read '%s'\")\n",argv[i]);
          exit(EXIT_FAILURE);
        }
        memset(l->i,0,sizeof(*l->i));
        l->i->type = file_in;
        l->i->ptr.file = input;
        repl(l);
        fclose(input);
        break;
      case getopt_string_input: /*not implemented yet*/
        if(++i < argc){
          memset(l->i,0,sizeof(*l->i));
          l->i->type = string_in;
          l->i->ptr.string = argv[i];
          l->i->max = strlen(argv[i]);
          printf("(input 'string \"%s\")\n",argv[i]);
          repl(l);
        } else {
          fprintf(stderr,"(error \"fatal: expecting arg after -e\" \"%s\" %d)\n",
              __FILE__,__LINE__);
          exit(EXIT_FAILURE);
        }
      break;
      case getopt_output_file:
        if(++i < argc){
          printf("(output 'file \"%s\")\n",argv[i]);
          if(NULL == (output = fopen(argv[i],"w"))){
            fprintf(stderr,"(error \"unable to write to '%s'\")\n",argv[i]);
            exit(EXIT_FAILURE);
          }
          l->o->type = file_out;
          l->o->ptr.file = output;
        } else {
          fprintf(stderr,"(error \"fatal: expecting arg after -o\" \"%s\" %d)\n",
              __FILE__,__LINE__);
          exit(EXIT_FAILURE);
        }
      break;
      case getopt_error:
      default:
        fprintf(stderr,"(error \"fatal: invalid command opts\" \"%s\" %d)\n",
            __FILE__,__LINE__);
        exit(EXIT_FAILURE);
    }
  }

  memset(l->i,0,sizeof(*l->i));
  l->i->type = file_in;
  l->i->ptr.file = stdin;
  repl(l);

  if(true == printGlobals_f){
    print_expr(l->global,l->o,0,l->e);
  }

  endlisp(l);

  return EXIT_SUCCESS;
}

