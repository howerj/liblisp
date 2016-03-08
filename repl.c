/** @file       repl.c
 *  @brief      An example REPL for the liblisp interpreter
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *
 *  @bug  The -H flag causes the interpreter to halt on an error, however
 *        this will also cause it to halt if an eval ran from within the
 *        interpreter fails.
 **/

#include "liblisp.h"
#include "private.h"
#include <stdlib.h>
#include <string.h>

/**** The need putting here by the build system / version control system ****/
#ifndef VERSION
#define VERSION unknown    /**< Version of the interpreter*/
#endif
#ifndef VCS_COMMIT
#define VCS_COMMIT unknown /**< Version control commit of this interpreter*/
#endif
#ifndef VCS_ORIGIN
#define VCS_ORIGIN unknown /**< Version control repository origin*/
#endif
/****************************************************************************/

static const char *usage = /**< command line options for example interpreter*/
    "(-[hcpvVEH])* (-[i\\-] file)* (-e string)* (-o file)* file* -";

static const char *help =
"The liblisp library and interpreter. For more information on usage\n\
consult the man pages 'lisp' and 'liblisp'. Alternatively, consult:\n\
\n\
	https://github.com/howerj/liblisp\n\
	http://work.anapnea.net/html/html/projects.html\n\
";

static unsigned lisp_verbosity = LISP_LOG_LEVEL_ERROR;

enum { go_switch,	     /**< current argument was a valid flag*/
	go_in_file,	     /**< current argument is file input to eval*/
	go_in_file_next_arg, /**< process the next argument as file input*/
	go_out_file,	     /**< next argument is an output file*/
	go_in_string,	     /**< next argument is a string to eval*/
	go_in_stdin,	     /**< read input from stdin*/
	go_error	     /**< invalid flag or argument*/
}; /**< getoptions enum*/

static int getoptions(lisp_t * l, char *arg, char *arg_0)
{ /**@brief simple parser for command line options**/
	int c;
	if ('-' != *arg++)
		return go_in_file;
	if (!arg[0])
		return go_in_stdin;
	while ((c = *arg++))
		switch (c) {
		case 'i':
		case '-':
			return go_in_file_next_arg;
		case 'h':
			printf("usage %s %s\n\n", arg_0, usage);
			puts(help);
			exit(0);
			break;
		case 'c':
			lisp_log_note(l, "'color-on");
			l->color_on = 1;
			break;	/*colorize output */
		case 'p':
			lisp_log_note(l, "'prompt-on");
			l->prompt_on = 1;
			break;	/*turn standard prompt when reading stdin */
		case 'E':
			lisp_log_note(l, "'line-editor-on");
			l->editor_on = 1;
			break;	/*turn line editor on when reading stdin */
		case 'H':
			lisp_log_note(l, "'halt-on-error");
			l->errors_halt = 1;
			break;
		case 'v':
			lisp_verbosity++;
			if(lisp_verbosity < LISP_LOG_LEVEL_LAST_INVALID)
				lisp_set_log_level(l, lisp_verbosity);
			else
				lisp_log_note(l, "'verbosity \"already set to maximum\"");
			break;
		case 'V':
			puts("program: liblisp");
			puts("version: " XSTRINGIFY(VERSION));
			puts("commit:  " XSTRINGIFY(VCS_COMMIT));
			puts("origin:  " XSTRINGIFY(VCS_ORIGIN));
			exit(0);
			break;
		case 'e':
			return go_in_string;
		case 'o':
			return go_out_file;
		default:
			fprintf(stderr, "unknown option '%c'\n", c);
			fprintf(stderr, "usage %s %s\n", arg_0, usage);
			return go_error;
		}
	return go_switch;	/*this argument was a valid flag, nothing more */
}

int lisp_repl(lisp_t * l, char *prompt, int editor_on)
{
	lisp_cell_t *ret;
	io_t *ofp, *efp;
	char *line = NULL;
	int r = 0;
	ofp = lisp_get_output(l);
	efp = lisp_get_logging(l);
	ofp->pretty = efp->pretty = 1;
	ofp->color = efp->color = l->color_on;
	if ((r = setjmp(l->recover)) < 0) {	/*catch errors and "sig" */
		l->recover_init = 0;
		return r;
	}
	l->recover_init = 1;
	if (editor_on && l->editor) {	/*handle line editing functionality */
		while ((line = l->editor(prompt))) {
			lisp_cell_t *prn;
			if (!line[strspn(line, " \t\r\n")]) {
				free(line);
				continue;
			}
			if (!(prn = lisp_eval_string(l, line))) {
				free(line);
				LISP_RECOVER(l, "\"%s\"", "invalid or incomplete line");
			}
			lisp_print(l, prn);
			free(line);
			line = NULL;
		}
	} else {		/*read from input with no special handling, or a file */
		for (;;) {
			/**@bug this should exit with a failure if not reading
                         *      from stdin and an error occurs, otherwise the
                         *      parser looses track, this is mainly a concern
                         *      for string input, where is makes for very
                         *      confusing behavior*/
			lisp_printf(l, ofp, 0, "%s", prompt);
			if (!(ret = reader(l, lisp_get_input(l))))
				break;
			if (!(ret = eval(l, 0, ret, l->top_env)))
				break;
			lisp_printf(l, ofp, 0, "%S\n", ret);
			l->gc_stack_used = 0;
		}
	}
	l->gc_stack_used = 0;
	l->recover_init = 0;
	return r;
}

int main_lisp_env(lisp_t * l, int argc, char **argv)
{
	int i, stdin_off = 0;
	lisp_cell_t *ob = l->nil;
	if (!l)
		return -1;
	for (i = argc - 1; i + 1; i--)	/*add command line args to list */
		if (!(ob = cons(l, mk_str(l, lstrdup_or_abort(argv[i])), ob)))
			return -1;
	if (!lisp_extend_top(l, lisp_intern(l, lstrdup_or_abort("args")), ob))
		return -1;

        lisp_add_cell(l, "*version*",           mk_str(l, lstrdup_or_abort(XSTRINGIFY(VERSION))));
        lisp_add_cell(l, "*commit*",            mk_str(l, lstrdup_or_abort(XSTRINGIFY(VCS_COMMIT))));
        lisp_add_cell(l, "*repository-origin*", mk_str(l, lstrdup_or_abort(XSTRINGIFY(VCS_ORIGIN))));

	for (i = 1; i < argc; i++)
		switch (getoptions(l, argv[i], argv[0])) {
		case go_switch:
			break;
		case go_in_stdin:	/*read from standard input */
			lisp_log_note(l, "'input-file 'stdin");
			io_close(lisp_get_input(l));
			if (lisp_set_input(l, io_fin(stdin)) < 0)
				return perror("stdin"), -1;
			if (lisp_repl(l, l->prompt_on ? "> " : "", l->editor_on) < 0)
				return -1;
			io_close(lisp_get_input(l));
			lisp_set_input(l, NULL);
			stdin_off = 1;
			break;
		case go_in_file_next_arg:
			if (!(++i < argc))
				return fprintf(stderr, "-i and -- expects file\n"), -1;
			/*--- fall through ---*/
		case go_in_file:	/*read from a file */
			lisp_log_note(l, "'input-file \"%s\"", argv[i]);
			io_close(lisp_get_input(l));
			if (lisp_set_input(l, io_fin(fopen(argv[i], "rb"))) < 0)
				return perror(argv[i]), -1;
			if (lisp_repl(l, "", 0) < 0)
				return -1;
			io_close(lisp_get_input(l));
			lisp_set_input(l, NULL);
			stdin_off = 1;
			break;
		case go_in_string:	/*evaluate a string */
			lisp_log_note(l, "'input-string \"%s\"", argv[i]);
			io_close(lisp_get_input(l));
			if (!(++i < argc))
				return fprintf(stderr, "-e expects arg\n"), -1;
			if (lisp_set_input(l, io_sin(argv[i])) < 0)
				return perror(argv[i]), -1;
			if (lisp_repl(l, "", 0) < 0)
				return -1;
			io_close(lisp_get_input(l));
			lisp_set_input(l, NULL);
			stdin_off = 1;
			break;
		case go_out_file:	/*change the file to write to */
			lisp_log_note(l, "'output-file \"%s\"", argv[i]);
			if (!(++i < argc))
				return fprintf(stderr, "-o expects arg\n"), -1;
			io_close(lisp_get_output(l));
			lisp_set_output(l, NULL);
			if (lisp_set_output(l, io_fout(fopen(argv[i], "wb"))) < 0)
				return perror(argv[i]), -1;
			break;
		case go_error:
		default:
			exit(-1);
		}
	if (!stdin_off) {
		lisp_log_note(l, "\"%s\"", "reading from stdin");
		if (lisp_repl(l, l->prompt_on ? "> " : "", l->editor_on) < 0)
			return -1;
	}
	lisp_destroy(l);
	return 0;
}

int main_lisp(int argc, char **argv)
{
	lisp_t *l;
	if (!(l = lisp_init()))
		return -1;
	return main_lisp_env(l, argc, argv);
}

