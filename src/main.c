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
#include "type.h"
#include "mem.h"
#include "sexpr.h"
#include "lisp.h"

typedef enum{
  getopt_switch,
  getopt_input_file,
  getopt_output_file,
  getopt_string_input,
  getopt_error
} getopt_e;

static int getopt(char *arg);
static int repl(lisp l);

static bool printGlobals_f = false;
static char *usage = "./lisp -hVG <file>\n";

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
  -h      Print this help message.\n\
  -V      Print version number.\n\
  -G      Print a list of all globals on normal program exit.\n\
  <file>  Read from <file> instead of stdin.\n\
";

/** 
 *  @brief    Process options; caution - may use exit()!
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
  FILE *input;

  /** setup environment */
  l = initlisp();

  for(i = 1; i < argc; i++){
    switch(getopt(argv[i])){
      case getopt_switch:
        /*getopt_switch means getopt set some flags or printed something*/
        break;
      case getopt_input_file:
      { /*try to treat it as an output file*/
        printf("(file input \"%s\")\n",argv[i]);
        if(NULL == (input = fopen(argv[i],"r"))){
          fprintf(stderr,"(error \"unable to read '%s'\")\n",argv[i]);
          exit(EXIT_FAILURE);
        }

        l->i->ptr.file = input;
        repl(l);
        fclose(input);
      }
      break;
      case getopt_output_file:
      case getopt_string_input:
      case getopt_error:
      default:
        exit(EXIT_FAILURE);
    }
  }

  l->i->ptr.file = stdin;
  repl(l);

  if(true == printGlobals_f){
    print_expr(l->global,l->o,0,l->e);
  }

  endlisp(l);

  return EXIT_SUCCESS;
}

