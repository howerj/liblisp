/** @file       liblisp_base.c
 *  @brief      Miscellaneous functions
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *
 *  @todo Add lock type and threading?
 **/
#include <liblisp.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <ctype.h>
#include <time.h>

#define SUBR_ISX(NAME)\
static lisp_cell_t *subr_ ## NAME (lisp_t *l, lisp_cell_t *args) { UNUSED(l);\
        char *s, c;\
        if(is_int(car(args)))\
                return NAME (get_int(car(args))) ? gsym_tee() : gsym_nil();\
        s = get_str(car(args));\
        if(!s[0]) return gsym_nil();\
        while((c = *s++)) \
                if(! NAME (c))\
                        return gsym_nil();\
        return gsym_tee();\
}

#define ISX_LIST\
        X(isalnum,  "Is a string or integer composed of alphanumeric characters?")\
        X(isalpha,  "Is a string or integer composed of alphabetic characters?")\
        X(iscntrl,  "Is a string or integer composed of control characters?")\
        X(isdigit,  "Is a string or integer composed of digits?")\
        X(isgraph,  "Is a string or integer composed of printable characters (excluding space)?")\
        X(islower,  "Is a string or integer composed of lower case characters?")\
        X(isprint,  "Is a string or integer composed of printable characters?")\
        X(ispunct,  "Is a string or integer composed of punctuation characters?")\
        X(isspace,  "Is a string or integer composed of whitespace characters?")\
        X(isupper,  "Is a string or integer composed of upper case characters?")\
        X(isxdigit, "Is a string or integer composed of hexadecimal digits?")

#define X(FUNC, IGNORE) SUBR_ISX(FUNC)
ISX_LIST /*defines lisp subroutines for checking whether a string only contains a character class*/
#undef X

#define SUBROUTINE_XLIST\
	X("timed-eval",  subr_timed_eval, "A",   "time an evaluation")\
	X("errno",       subr_errno,      "",    "return the current errno")\
	X("strerror",    subr_strerror,   "d",   "convert an errno into a string describing that error")\
	X("ilog2",       subr_ilog2,      "d",   "compute the binary logarithm of an integer")\
	X("random",      subr_rand,      "",     "return a pseudo random number generator")\
	X("ipow",        subr_ipow,       "d d", "compute the integer exponentiation of two numbers")\
	X("seed",        subr_seed,      "d d",  "seed the pseudo random number generator")\
	X("strstr",      subr_strstr,    "Z Z",  "return offset of first occurrence of second string in the first")\
	X("strcspn",     subr_strcspn,   "Z Z",  "offset into first string of first occurrence of character in second string")\
	X("strspn",      subr_strspn,   "Z Z",  "offset into first string of last occurrence of character in second string")\
	X("time",        subr_time,      "",     "create a list representing the time")\
	X("date",        subr_date,      "",     "return a list representing the date (GMT) (not thread safe)")\


#define X(NAME, SUBR, VALIDATION , DOCSTRING) static lisp_cell_t * SUBR (lisp_t *l, lisp_cell_t *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X

#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(NAME, DOCSTRING), SUBR },
static lisp_module_subroutines_t primitives[] = {
	SUBROUTINE_XLIST		/*all of the subr functions */
	{ NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
}
;
#undef X

static int32_t ilog2(uint64_t v)
{
	if (!v)
		return INT32_MIN;
	int32_t r = 0;
	while (v >>= 1)
		r++;
	return r;
}

static uint64_t ipow(uint64_t base, uint64_t exp)
{
	uint64_t result = 1;
	while (exp) {
		if (exp & 1)
			result *= base;
		exp >>= 1;
		base *= base;
	}
	return result;
}

static uint64_t xorshift128plus(uint64_t s[2])
{
	/*PRNG*/ uint64_t x = s[0];
	uint64_t const y = s[1];
	s[0] = y;
	x ^= x << 23;		/*a */
	x ^= x >> 17;		/*b */
	x ^= y ^ (y >> 26);	/*c */
	s[1] = x;
	return x + y;
}

static lisp_cell_t *subr_ilog2(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, ilog2(get_int(car(args))));
}

static lisp_cell_t *subr_ipow(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, ipow(get_int(car(args)), get_int(CADR(args))));
}

/**@todo lock this */
static uint64_t random_state[2] /**< PRNG state*/;
static lisp_cell_t *subr_rand(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_int(l, xorshift128plus(random_state));
}

static lisp_cell_t *subr_seed(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	random_state[0] = get_int(car(args));
	random_state[1] = get_int(CADR(args));
	return gsym_tee();
}

static lisp_cell_t *subr_timed_eval(lisp_t * l, lisp_cell_t * args)
{
	clock_t start, end;
	lisp_float_t used;
	lisp_cell_t *x;
	start = clock();
	x = lisp_eval(l, car(args)); /**@todo pass in environment*/
	end = clock();
	used = ((lisp_float_t) (end - start)) / CLOCKS_PER_SEC;
	return cons(l, mk_float(l, used), x);
}

static lisp_cell_t *subr_errno(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	int e = errno;
	errno = 0;
	return mk_int(l, e);
}

static lisp_cell_t *subr_strerror(lisp_t * l, lisp_cell_t * args)
{
	return mk_str(l, lisp_strdup(l, strerror(get_int(car(args)))));
}

static lisp_cell_t *subr_strstr(lisp_t *l, lisp_cell_t *args)
{
	char *s = get_str(car(args));
	char *p = get_str(CADR(args));
	char *r = strstr(s, p);
	if(!r)
		return gsym_nil();
	return mk_int(l, r-s);
}

static lisp_cell_t *subr_strcspn(lisp_t *l, lisp_cell_t *args)
{
	return mk_int(l, strcspn(get_str(car(args)), get_str(CADR(args))));
}

static lisp_cell_t *subr_strspn(lisp_t *l, lisp_cell_t *args)
{
	return mk_int(l, strspn(get_str(car(args)), get_str(CADR(args))));
}

static lisp_cell_t *subr_time(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_int(l, time(NULL));
}



static lisp_cell_t *subr_date(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	time_t raw;
	struct tm *gt;
	/**@todo locking */
	time(&raw);
	gt = gmtime(&raw);
	return mk_list(l, mk_int(l, gt->tm_year + 1900), mk_int(l, gt->tm_mon),
		       mk_int(l, gt->tm_wday), mk_int(l, gt->tm_mday),
		       mk_int(l, gt->tm_hour), mk_int(l, gt->tm_min), mk_int(l, gt->tm_sec), NULL);
}

int lisp_module_initialize(lisp_t *l)
{
	assert(l);

        random_state[0] = 0xCAFEBABE; /*Are these good seeds?*/
        random_state[1] = 0xDEADC0DE;
        for(size_t i = 0; i < 4096; i++) /*discard first N numbers*/
                (void)xorshift128plus(random_state);

#define X(FUNC, DOCSTRING) if(!lisp_add_subr(l, # FUNC "?", subr_ ## FUNC, "C", MK_DOCSTR(# FUNC "?", DOCSTRING))) goto fail;
ISX_LIST /*add all of the subroutines for string character class testing*/
#undef X

	return lisp_add_module_subroutines(l, primitives, 0);
fail:
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__ ((constructor));
static void destruct(void) __attribute__ ((destructor));
static void construct(void) {}
static void destruct(void)  {}
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
