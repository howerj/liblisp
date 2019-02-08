/** @file       liblisp_pcre.c
 *  @brief      Perl compatible regular expressions
 *  @author     Richard Howe (2016)
 *  @license    LGPL v2.1 or Later
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html>
 *  @email      howe.r.j.89@gmail.com
 *  @note       "int" has to be used in various places to conform to the
 *              PCRE API.
 *  @note       Some more reading up about the PCRE library needs doing,
 *              however, it seems to work, there will be edge cases where it
 *              does not.
 **/
#include <assert.h>
#include <lispmod.h>
#include <string.h>
#include <pcre.h>
#include <assert.h>
#include <limits.h>

#define OVECTOR_SIZE (99)

#define SUBROUTINE_XLIST\
        X("regex",      subr_regex,      "Z Z", "search for a pattern in a string")\
        X("regex-span", subr_regex_span, "Z Z", "search for a pattern in a string, returning a list of offsets for matches")\
        X("split",      subr_split,      "Z Z", "split a string based on a pattern")\
        X("split-span", subr_split_span, "Z Z", "split a string based on a pattern, returning a list of offsets")\

#define X(NAME, SUBR, VALIDATION, DOCSTRING) static lisp_cell_t * SUBR (lisp_t *l, lisp_cell_t *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(#SUBR, DOCSTRING), SUBR },
static lisp_module_subroutines_t primitives[] = {
	SUBROUTINE_XLIST	/*all of the subr functions */
	{NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};
#undef X

static lisp_cell_t *regex_engine_wrapper(lisp_t * l, lisp_cell_t * args, int split, int return_strings)
{
	pcre *compiled;
	char *regex = get_sym(car(args)),
		*string = get_sym(CADR(args)),
		*last_string = NULL;
	const char *errstr = NULL;
	int error_offset = 0,
	    ret = 0,
	    last_string_len = 0;
	int ovector[OVECTOR_SIZE]; /*meant to be a multiple of 3 according to docs*/
	unsigned string_offset = 0,
		 string_length = get_length(CADR(args));
	lisp_cell_t *x, *head;


	if(!(compiled = pcre_compile(regex, 0, &errstr, &error_offset, NULL))) {
		lisp_log_error(l, "%y'pcre-error 'compile %r\"%s\" %r\"%s\"%t", regex, errstr ? errstr : "(nil)");
		return gsym_error();
	}

	head = x = cons(l, gsym_nil(), gsym_nil());
	while((string_offset < string_length)
	&& (ret = pcre_exec(compiled, NULL, string, string_length, string_offset, 0, ovector, OVECTOR_SIZE))) {
		int next = 0; /*when matching "c*" the offset is not advanced, next is set to one later so it is*/
		assert(string_offset < INT_MAX);
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
			assert(end >= start);
		       	assert(size >= 0);
			if(size == 0) {
				next = 1;
			}
			if(return_strings) {
				int asize = split ? (int)(start - string_offset) : size;
				int astart = split ? (int)string_offset : start;
				assert(asize >= 0);
				assert(astart >= 0);
				char *s = malloc(asize + 1);
				if(!s)
					LISP_HALT(l, "\"%s\"", "out of memory");
				s[asize] = '\0';
				memcpy(s, string + astart, asize);
				/*printf(">%.*s\n", asize, string + astart);*/
				set_cdr(x, cons(l, mk_str(l, s), gsym_nil()));
			} else {
				int astart = split ? (int)string_offset : start;
				int aend   = split ? (int)(astart + start - string_offset) : end;
				assert(astart >= 0);
				assert(aend >= 0);
				set_cdr(x,
					cons(l,
						mk_list(l, mk_int(l, astart), mk_int(l, aend), NULL),
					       	gsym_nil()));
			}
			x = cdr(x);
		}
		string_offset = ovector[1] + next;/*???*/

		last_string_len = string_length - string_offset;
		last_string = string + string_offset;
	}

nomatch:
	/*deal with last entry when splitting strings*/
	if(split && last_string) {
		if(return_strings) {
			assert(last_string_len > 0);
			char *s = malloc(last_string_len + 1);
			s[last_string_len] = '\0';
			memcpy(s, last_string, last_string_len);
			set_cdr(x, cons(l, mk_str(l, s), gsym_nil()));
		} else {
			int start = last_string - string;
			int end = start + last_string_len;
			assert(start >= 0);
			assert(end >= 0);
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

static lisp_cell_t *subr_regex_span(lisp_t * l, lisp_cell_t * args)
{
	return regex_engine_wrapper(l, args, 0, 0);
}

static lisp_cell_t *subr_regex(lisp_t * l, lisp_cell_t * args)
{
	return regex_engine_wrapper(l, args, 0, 1);
}

static lisp_cell_t *subr_split_span(lisp_t * l, lisp_cell_t * args)
{
	return regex_engine_wrapper(l, args, 1, 0);
}

static lisp_cell_t *subr_split(lisp_t * l, lisp_cell_t * args)
{
	/*split should default to splitting on whitespace, which is does not do*/
	return regex_engine_wrapper(l, args, 1, 1);
}

int lisp_module_initialize(lisp_t *l)
{
	assert(l);
	if(lisp_add_module_subroutines(l, primitives, 0) < 0)
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
