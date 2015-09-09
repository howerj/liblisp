/** @file       repl.c
 *  @brief      An example REPL for the liblisp interpreter
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com**/


#include "liblisp.h"
#include "private.h"
#include <stdlib.h>
#include <string.h>

static char *usage = /**< command line options for example interpreter*/
     "usage: %s (-[hcpEH])* (-[i\\-] file)* (-e string)* (-o file)* file* -\n";

enum {  go_switch,           /**< current argument was a valid flag*/
        go_in_file,          /**< current argument is file input to eval*/
        go_in_file_next_arg, /**< process the next argument as file input*/
        go_out_file,         /**< next argument is an output file*/
        go_in_string,        /**< next argument is a string to eval*/
        go_in_stdin,         /**< read input from stdin*/
        go_error             /**< invalid flag or argument*/
}; /**< getoptions enum*/

static int getoptions(lisp *l, char *arg, char *arg_0)
{ /**@brief simple parser for command line options**/
        int c;
        if('-' != *arg++) return go_in_file;
        if(!arg[0]) return go_in_stdin;
        while((c = *arg++))
                switch(c) {
                case 'i': case '-': return go_in_file_next_arg;
                case 'h': printf(usage, arg_0); exit(0); break;
                case 'c': l->color_on  = 1; break; /*colorize output*/
                case 'p': l->prompt_on = 1; break; /*turn standard prompt when reading stdin*/
                case 'E': l->editor_on = 1; break; /*turn line editor on when reading stdin*/
                case 'H': l->errors_halt = 1; break;
                case 'e': return go_in_string; 
                case 'o': return go_out_file; 
                default:  fprintf(stderr, "unknown option '%c'\n", c);
                          fprintf(stderr, usage, arg_0);
                          return go_error; 
                }
        return go_switch; /*this argument was a valid flag, nothing more*/
}

int lisp_repl(lisp *l, char *prompt, int editor_on) {
        cell *ret;
        char *line = NULL;
        int r = 0;
        l->ofp->pretty = l->efp->pretty = 1; /*pretty print output*/
        l->ofp->color = l->efp->color = l->color_on;
        if((r = setjmp(l->recover)) < 0) {  /*catch errors and "sig"*/
                l->recover_init = 0;
                return r; 
        }
        l->recover_init = 1;
        if(editor_on && l->editor) { /*handle line editing functionality*/
                while((line = l->editor(prompt))) {
                        cell *prn; 
                        if(!line[strspn(line, " \t\r\n")]) { free(line); continue; }
                        if(!(prn = lisp_eval_string(l, line))) {
                                free(line);
                                RECOVER(l, "\"%s\"", "invalid or incomplete line");
                        }
                        lisp_print(l, prn);
                        free(line);
                        line = NULL;
                }
        } else { /*read from stdin with no special handling, or a file*/
                for(;;){
                        printerf(l, l->ofp, 0, "%s", prompt);
                        if(!(ret = reader(l, l->ifp))) break;
                        if(!(ret = eval(l, 0, ret, l->top_env))) break;
                        printerf(l, l->ofp, 0, "%S\n", ret);
                        l->gc_stack_used = 0;
                }
        }
        l->gc_stack_used = 0;
        l->recover_init = 0;
        return r;
}

int main_lisp_env(lisp *l, int argc, char **argv) {
        int i, stdin_off = 0;
        cell *ob = l->nil;
        if(!l) return -1;
        for(i = argc - 1; i + 1 ; i--) /*add command line args to list*/
                if(!(ob = cons(l, mkstr(l, lstrdup(argv[i])), ob))) 
                        return -1; 
        if(!extend_top(l, intern(l, lstrdup("args")), ob))
                return -1;
        for(i = 1; i < argc; i++)
                switch(getoptions(l, argv[i], argv[0])) {
                case go_switch: break;
                case go_in_stdin: /*read from standard input*/
                        io_close(l->ifp);
                        if(!(l->ifp = io_fin(stdin)))
                                return perror("stdin"), -1;
                        if(lisp_repl(l, l->prompt_on ? "> ": "", l->editor_on) < 0) 
                                return -1;
                        io_close(l->ifp);
                        l->ifp = NULL;
                        stdin_off = 1;
                        break;
                case go_in_file_next_arg: 
                        if(!(++i < argc))
                                return fprintf(stderr, "-i and -- expects file\n"), -1;
                        /*--- fall through ---*/
                case go_in_file: /*read from a file*/
                        io_close(l->ifp);
                        if(!(l->ifp = io_fin(fopen(argv[i], "rb"))))
                                return perror(argv[i]), -1;
                        if(lisp_repl(l, "", 0) < 0) return -1;
                        io_close(l->ifp);
                        l->ifp = NULL;
                        stdin_off = 1;
                        break;
                case go_in_string: /*evaluate a string*/
                        io_close(l->ifp);
                        if(!(++i < argc))
                                return fprintf(stderr, "-e expects arg\n"), -1;
                        if(!(l->ifp = io_sin(argv[i])))
                                return perror(argv[i]), -1;
                        if(lisp_repl(l, "", 0) < 0) return -1;
                        io_close(l->ifp);
                        l->ifp = NULL;
                        stdin_off = 1;
                        break;
                case go_out_file: /*change the file to write to*/
                        if(!(++i < argc))
                                return fprintf(stderr, "-o expects arg\n"), -1;
                        io_close(l->ofp);
                        if(!(l->ofp = io_fout(fopen(argv[i], "wb"))))
                                return perror(argv[i]), -1;
                        break;
                case go_error:
                default: exit(-1);
                }
        if(!stdin_off)
                if(lisp_repl(l, l->prompt_on ? "> ": "", l->editor_on) < 0) return -1;
        lisp_destroy(l);
        return 0;
}

int main_lisp(int argc, char **argv) {
        lisp *l;
        if(!(l = lisp_init())) return -1;
        return main_lisp_env(l, argc, argv);
}

