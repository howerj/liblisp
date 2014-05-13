/**
 *  @file           main.c
 *  @brief          Driver for the lisp interpreter. Contains main();
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v3.0
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
#include "sexpr.h"
#include "lisp.h"

static int getopt(char *arg);
static int repl(lisp l);

static bool printGlobals_f = false;
static char *usage = "./lisp -hVG <file>";

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
Usage:\n\
  ./lisp -hVG <file>\n\
\n\
  -h      Print this help message.\n\
  -V      Print version number.\n\
  -G      Print a list of all globals on normal program exit.\n\
  <file>  Read from <file> instead of stdin.\n\
";

/** 
 *  @brief          Process options; caution - may use exit()!
 *  @param          arg   current command line argument
 *  @return         0 = arg was a switch, 1 arg could be a file
 *                  exit() if arg was a switch but is an invalid
 *                  switch.
 */
static int getopt(char *arg){
  int c;

  if('-' != *arg++){ /** @todo open arg as file */
    return 1;
  }

  while((c = *arg++)){
    switch(c){
      case 'h':
        printf("%s",help);
        break;
      case 'V':
        printf("%s",version);
        break;
      case 'G':
        printGlobals_f = true;
        break;
      default:
        fprintf(stderr,"unknown option: '%c'\n%s\n", c, usage);
        exit(EXIT_FAILURE);
    }
  }
  return 0;
}

static int repl(lisp l){
  expr x;
  while(NULL != (x = parse_term(l->i, l->e))){
    x = eval(x,l->env,l);
    print_expr(x,l->o,0,l->e);
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
    if(1 == getopt(argv[i])){
      printf("(file input \"%s\")\n",argv[i]);
      if(NULL == (input = fopen(argv[i],"r"))){
        fprintf(stderr,"(error \"unable to read '%s'\")\n",argv[i]);
        exit(EXIT_FAILURE);
      }

      l->i->ptr.file = input;
      repl(l);
      fclose(input);
    }
  }

  l->i->ptr.file = stdin;
  repl(l);

  if(true == printGlobals_f){
    print_expr(l->global,l->o,0,l->e);
  }
  return EXIT_SUCCESS;
}

