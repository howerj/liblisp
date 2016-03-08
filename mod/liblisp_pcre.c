/** @file       liblisp_pcre.c
 *  @brief      Perl compatible regular expressions
 *  @author     Richard Howe (2016)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *  @warning    "int" has to be used in various places to conform to the
 *              PCRE API.
 **/
#include <assert.h>
#include <liblisp.h>
#include <string.h>
#include <pcre.h>

#define OVECTOR_SIZE (99)

#define SUBROUTINE_XLIST\
        X("regex", subr_regex,      "Z Z b b", "Perl Compatible Regular Expression regex function")\

#define X(NAME, SUBR, VALIDATION, DOCSTRING) static lisp_cell_t * SUBR (lisp_t *l, lisp_cell_t *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(#SUBR, DOCSTRING), SUBR },
static struct module_subroutines {
	char *name, *validate, *docstring;
	subr p;
} primitives[] = {
	SUBROUTINE_XLIST	/*all of the subr functions */
	{NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};
#undef X

static lisp_cell_t *subr_regex(lisp_t * l, lisp_cell_t * args)
{
	pcre *compiled;
	char *regex = get_sym(car(args));
	char *string = get_sym(CADR(args));
	const char *errstr = NULL;
	int error_offset = 0, ret = 0;
	int ovector[OVECTOR_SIZE]; /*meant to be a multiple of 3 according to docs*/
	unsigned string_offset = 0, string_length = get_length(CADR(args));
	lisp_cell_t *x, *head;
	int return_strings = CADDR(args) == gsym_tee() ? 1 : 0;
	int split = CADDDR(args) == gsym_tee() ? 1 : 0;
	char *last_string = NULL;
	int last_string_len = 0;
	

	if(!(compiled = pcre_compile(regex, 0, &errstr, &error_offset, NULL))) {
		lisp_log_error(l, "%y'pcre-error 'compile %r\"%s\" %r\"%s\"%t", regex, errstr ? errstr : "(nil)");
		return gsym_error();
	}

	head = x = cons(l, gsym_nil(), gsym_nil());
	while((string_offset < string_length) 
	&& (ret = pcre_exec(compiled, NULL, string, string_length, string_offset, 0, ovector, OVECTOR_SIZE))) {
		if(ret < 0) { /*error handler*/
			switch(ret) {
			case PCRE_ERROR_NOMATCH: 
				goto nomatch;
			case PCRE_ERROR_NULL: 
				lisp_log_error(l,"%y'pcre-error %r\"unexpected null\"%t");
				goto fail;
			case PCRE_ERROR_BADOPTION: 
				lisp_log_error(l,"%y'pcre-error %r\"bad option\"%t");
				goto fail;
			case PCRE_ERROR_BADMAGIC: 
				lisp_log_error(l,"%y'pcre-error %r\"bad magic number\"%t");
				goto fail;
			case PCRE_ERROR_UNKNOWN_NODE: 
				lisp_log_error(l,"%y'pcre-error %r\"bad compilation\"%t");
				goto fail;
			case PCRE_ERROR_NOMEMORY: 
				lisp_log_error(l,"%y'pcre-error %r\"ran out of memory\"%t");
				goto fail;
			default:
				lisp_log_error(l,"%y'pcre-error %r\"unknown error\"%t");
				goto fail;
			}
		}

		/*we must have a match then*/
		for(int i = 0; i < ret; i++) {
			int start = ovector[2*i];
			int end   = ovector[2*i+1];
			int size  = end - start;
			assert(end > start);
			if(return_strings) {
				int asize = split ? start - string_offset : size;
				int astart = split ? string_offset : start;
				char *s = malloc(asize + 1);
				if(!s)
					LISP_HALT(l, "\"%s\"", "out of memory");
				s[asize] = '\0';
				memcpy(s, string + astart, asize);
				set_cdr(x, cons(l, mk_str(l, s), gsym_nil()));
			} else {
				int astart = split ? string_offset : start;
				int aend   = split ? astart + start - string_offset: end;
				set_cdr(x, 
					cons(l, 
						mk_list(l, mk_int(l, astart), mk_int(l, aend), NULL),
					       	gsym_nil()));
			}
			x = cdr(x);
		}
		string_offset = ovector[1];/*???*/

		last_string_len = string_length - string_offset;
		last_string = string + string_offset;
	}

nomatch:
	/*deal with last entry when splitting strings*/
	if(split && last_string) {
		if(return_strings) {
			char *s = malloc(last_string_len + 1);
			s[last_string_len] = '\0';
			memcpy(s, last_string, last_string_len);
			set_cdr(x, cons(l, mk_str(l, s), gsym_nil()));
		} else {
			int start = last_string - string;
			int end = start + last_string_len;
			set_cdr(x,
				cons(l,
					mk_list(l, mk_int(l, start), mk_int(l, end), NULL),
					gsym_nil()));
			x = cdr(x);
		}
	}
	pcre_free(compiled);
	return cdr(head);
fail:
	pcre_free(compiled);
	return gsym_error();
}

int lisp_module_initialize(lisp_t *l)
{
	size_t i;
	assert(l);
	for (i = 0; primitives[i].p; i++)	/*add all primitives from this module */
		if (!lisp_add_subr(l, primitives[i].name, primitives[i].p, primitives[i].validate, primitives[i].docstring))
			goto fail;
	return 0;
 fail:	
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__ ((constructor));
static void destruct(void) __attribute__ ((destructor));
static void construct(void) {}
static void destruct(void) {}
#elif _WIN32
#include <windows.h>
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved)
{
	UNUSED(hinstDLL);
	UNUSED(lpvReserved);
	switch (fdwReason) {
	case DLL_PROCESS_ATTACH:
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
