/** @file       liblisp_line.c
 *  @brief      Line editor module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *  @bug	Needs to be ported to Windows!
 *  **/
#include <assert.h>
#include <libline.h>
#include <lispmod.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

static char *histfile = ".lisphist";
static char *homedir;
static int running; /**< only handle errors when the lisp interpreter is running*/
static lisp_t *locked_lisp; 
static lisp_mutex_t mutex_single_threaded_module = LISP_MUTEX_INITIALIZER;

/** @brief This function tells a running lisp REPL to halt if it is reading
 *         input or printing output, but if it is evaluating an expression
 *         instead it will stop doing that and try to get more input. This is
 *         so the user can sent the SIGINT signal to the interpreter to halt
 *         expressions that are not terminating, but can still use SIGINT to
 *         halt the interpreter by at maximum sending the signal twice. This
 *         function is meant to be passed to "signal".
 *  @param sig This will only be SIGINT in our case. */
static void sig_int_handler(int sig)
{
	if (!running || !locked_lisp)
		exit(0);	/*exit if lisp environment is not running */
	lisp_set_signal(locked_lisp, sig);	/*notify lisp environment of signal */
	running = 0;
}

#define SUBROUTINE_XLIST\
	X("line-editor-mode", subr_line_editor_mode, "b", "set the line editor mode (t = vi-mode)")\
	X("clear-screen",     subr_clear_screen,     "",  "clear the screen")\
	X("history-length",   subr_hist_len,         "d", "set the length of the history file")\
	X("readline",         subr_readline,         "Z", "read a line of input with the libline library")


static void *get_hash_key(const char *key, void *val)
{
	if (!key || !val)
		return NULL;
	return (void *)key;
}

static void completion_callback(const char *line, size_t pos, line_completions * lc)
{
	assert(line && locked_lisp);
	hash_table_t *h;
	char *key, *comp, *linecopy, *prepend = NULL;
	size_t i, lastsep;
	if (!pos)
		return;
	h = get_hash(lisp_get_all_symbols(locked_lisp));

	for(lastsep = 0, i = 0; i < pos; i++)
		if(strchr(" \t{}()'\".", line[i]))
			lastsep = i;

	if(!(linecopy = calloc(pos - lastsep + 1, 1)))
		FATAL("out of memory");
	if(!(prepend  = calloc(lastsep + 2, 1)))
		FATAL("out of memory");
	memcpy(linecopy, line + lastsep + 1, pos - lastsep);
	memcpy(prepend,  line, lastsep + !!strchr(" \t{}()\".", line[lastsep]));

	if (!(comp = VSTRCAT("*", linecopy, "*"))) /*match any symbol with key in it */
		FATAL("out of memory");

	while ((key = (char *)hash_foreach(h, get_hash_key))) {
		if (match(comp, key) > 0) {
			char *add;
		       	if(!(add = VSTRCAT(prepend, key)))
				FATAL("out of memory");
			line_add_completion(lc, add);
			free(add);
		}
	}
	free(comp);
	free(prepend);
	free(linecopy);
}

static int i_want_more_lines(const char *line)
{
	return unbalanced(line, '(', ')') > 0 || unbalanced(line, '{', '}') > 0;
}

static char *line_editing_function(const char *prompt)
{
	static int warned = 0; /**< have we warned the user we cannot write to
                                    the history file?*/
	char *line, *new = NULL, *conc = NULL;
	char varprompt[128];
	int max_len = 0;
	running = 0; /*SIGINT handling off when reading input */
	line = line_editor(prompt);
	/*do not add blank lines */
	if (!line || !line[strspn(line, " \t\r\n")])
		return line;
	max_len = strlen(line);
	while (i_want_more_lines(line)) {
		sprintf(varprompt, "%*.*s", max_len+2, 120, "=>");
		if (!(new = line_editor(varprompt))) { 
			free(line);
			return NULL;
		}
		max_len += strspn(new, " "); 

		if (!(conc = VSTRCATSEP(" ", line, new))) {
			free(new);
			free(line);
			return NULL;
		}
		free(new);
		free(line);
		line = conc;
	}

	line_history_add(line);
	if (line_history_save(histfile) && !warned) {
		PRINT_ERROR("\"could not save history\" \"%s\"", histfile);
		warned = 1;
	}
	assert(locked_lisp);	/*l needed for signal handler */
	if (signal(SIGINT, sig_int_handler) == SIG_ERR)	/*(re)install handler */
		PRINT_ERROR("\"%s\"", "could not set signal handler");
	running = 1;		/*SIGINT handling on when evaluating input */
	return line;		/*returned line will be evaluated */
}

static lisp_cell_t *subr_line_editor_mode(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	line_set_vi_mode(is_nil(car(args)) ? 0 : 1);
	return gsym_tee();
}

static lisp_cell_t *subr_hist_len(lisp_t * l, lisp_cell_t * args)
{
	if (!line_history_set_maxlen((int)get_int(car(args))))
		LISP_HALT(l, "\"%s\"", "out of memory");
	return gsym_tee();
}

static lisp_cell_t *subr_clear_screen(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	UNUSED(args);
	line_clearscreen();
	return gsym_tee();
}

static lisp_cell_t *subr_readline(lisp_t * l, lisp_cell_t * args)
{
	char *prompt = get_str(car(args)), *line;
	return mk_str(l, (line = line_editing_function(prompt)) ? line : lisp_strdup(l, ""));
}

#define X(NAME, SUBR, VALIDATION, DOCSTRING) static lisp_cell_t * SUBR (lisp_t *l, lisp_cell_t *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(NAME, DOCSTRING), SUBR },
static lisp_module_subroutines_t primitives[] = {
	SUBROUTINE_XLIST	/*all of the subr functions */
	{NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};
#undef X

int lisp_module_initialize(lisp_t *l)
{
	char *sep = "/";
	assert(l);
	if(lisp_mutex_trylock(&mutex_single_threaded_module)) {
		lisp_log_error(l, "module: line editor load failure (module already in use)\n");
		return -1;
	}
	locked_lisp = l;

	if (signal(SIGINT, sig_int_handler) == SIG_ERR) {
		PRINT_ERROR("\"%s\"", "could not set signal handler");
		goto fail;
	}
	/* This adds line editor functionality from the "libline" library,
	 * this is a fork of the "linenoise" library.*/

	homedir = getenv("LISPHOME");
#ifdef __unix__
	if(!homedir)
		homedir = getenv("HOME");	/*Unix home path */
#elif _WIN32
	if(!homedir)
		homedir = getenv("HOMEPATH");
	sep = "\\";
#endif
	if (homedir)		/*if found put the history file there */
		histfile = VSTRCATSEP(sep, homedir, histfile);
	if (!histfile)
		PRINT_ERROR("\"%s\"", "VSTRCATSEP allocation failed");
	lisp_set_line_editor(l, line_editing_function);
	line_history_load(histfile);
	line_set_vi_mode(0);	/*start up in a lame editing mode thats not confusing */
	line_set_completion_callback(completion_callback);
	lisp_add_cell(l, "*history-file*", mk_str(l, lisp_strdup(l, histfile)));

	if(lisp_add_module_subroutines(l, primitives, 0) < 0)
		goto fail;
	return 0;
 fail:	
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__ ((constructor));
static void destruct(void) __attribute__ ((destructor));
static void construct(void) { }

static void destruct(void)
{
	/*lisp_set_line_editor(l, NULL); */
	if (homedir)
		free(histfile);
	/*unlock mutex*/
}
#elif _WIN32
#include <windows.h>
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNUSED(hinstDLL);
	UNUSED(lpvReserved);
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
		initialize();
		fprintf(stderr, "warning: the line editor module is very buggy under Windows\n");
		break;
	case DLL_PROCESS_DETACH:
		break;
	case DLL_THREAD_ATTACH:
		break;
	case DLL_THREAD_DETACH:
		break;
	default:
		break;
	}
	return TRUE;
}
#endif
