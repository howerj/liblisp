/**
 *  @file           main.c                                                   
 *  @brief          Driver for the lisp interpreter. Contains main();        
 *  @author         Richard James Howe.                                      
 *  @copyright      Copyright 2013 Richard James Howe.                       
 *  @license        GPL v3.0                                                 
 *  @email          howe.r.j.89@gmail.com                                    
 **/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "sexpr.h"
#include "lisp.h"

static int getopt(int argc, char *argv[]);

static bool printGlobals_f = false;

static char *usage = "./lisp -hViG <file>";

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
  ./lisp -hViG <file>\n\
\n\
  -h      Print this help message.\n\
  -V      Print version number.\n\
  -i      Input file.\n\
  -G      Print a list of all globals on normal program exit.\n\
  <file>  Iff -i given read from instead of stdin.\n\
";

/** TODO:
 *    - implement input file option
 *    - --""-- output --""--
 *    - execute on string passed in
 */
static int getopt(int argc, char *argv[]){
  int c;
  if(argc<=1)
    return 0;

  if('-' != *argv[1]++){ /** TODO: Open arg as file */
    return 0;
  }

  while((c = *argv[1]++)){
    switch(c){
      case 'h':
        printf("%s",help);
        return 0;
      case 'V':
        printf("%s",version);
        return 0;
      case 'i':
        break;
      case 'G':
        printGlobals_f = true;
        break;
      default:
        fprintf(stderr,"unknown option: '%c'\n", c);
        return 1;
    }
  }
  return 0;
}

int main(int argc, char *argv[]){
  lisp l;
  expr x,env=NULL;

  /** setup environment */
  l = initlisp();

  if(1<argc){
    if(getopt(argc,argv)){
        fprintf(stderr,"%s\n",usage);
        return EXIT_FAILURE;
    }
  } else {

  }

  while((x = parse_term(&l->i, &l->e))){
    x = eval(x,env,l);
    print_expr(x,&l->o,0,&l->e);
  }

  if(true == printGlobals_f){
    print_expr(l->global,&l->o,0,&l->e);
  }
  return EXIT_SUCCESS;
}

