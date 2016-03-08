/** @file       liblisp_curl.c
 *  @brief      Interface to curl https://curl.haxx.se/libcurl/
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *  @todo       Port to Windows
 **/
#include <assert.h>
#include <liblisp.h>
#include <curl/curl.h>
#include <time.h>
#ifdef __unix__
#include <pthread.h>
#else
#error "Unsupported platform!"
#endif

/**@note in "main.c" it would be nice to have functions for various different
 * types of modules that would allow exclusive access to that modules, or allow
 * for locked initialization as is happening here*/
static pthread_mutex_t curl_global_initialize_lock = PTHREAD_MUTEX_INITIALIZER;
static int locked_initialize_done = 0;
static int locked_initialize_failed = 0;

#define SUBROUTINE_XLIST\
	X("curl",         subr_curl,         "Z",  "")\
	X("curl-version", subr_curl_version, "",   "Curl version in use")\
	X("url-encode",   subr_url_encode,   "Z",  "URL encode a string")\
	X("url-decode",   subr_url_decode,   "Z",  "URL decode a string")\
	X("date-string-to-time", subr_curl_time,  "Z",  "Return time since the Epoch from a date string")

#define X(NAME, SUBR, VALIDATION, DOCSTRING) static lisp_cell_t * SUBR (lisp_t *l, lisp_cell_t *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(NAME, DOCSTRING), SUBR },
static lisp_module_subroutines_t primitives[] = {
	SUBROUTINE_XLIST	/*all of the subr functions */
	{ NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};
#undef X

static lisp_cell_t *subr_curl(lisp_t *l, lisp_cell_t *args)
{
	UNUSED(args);
	return mk_int(l, 0);
}

static lisp_cell_t *subr_url_encode(lisp_t *l, lisp_cell_t *args)
{
	lisp_cell_t *ret = gsym_error();
	CURL *c = curl_easy_init();
	char *rs = NULL;
	if(!c)
		return gsym_error();
	if(!(rs = curl_easy_escape(c, get_sym(car(args)), get_length(car(args)))))
		goto fail;
	ret = mk_str(l, lisp_strdup(l, rs));

	curl_free(rs);
fail:   curl_easy_cleanup(c);
	return ret;
}

static lisp_cell_t *subr_url_decode(lisp_t *l, lisp_cell_t *args)
{
	lisp_cell_t *ret = gsym_error();
	CURL *c = curl_easy_init();
	char *rs = NULL;
	if(!c)
		return gsym_error();
	/*although curl_easy_unescape can deal with strings with NUL in them,
	 *liblisp cannot at the moment*/
	if(!(rs = curl_easy_unescape(c, get_sym(car(args)), get_length(car(args)), NULL)))
		goto fail;
	ret = mk_str(l, lisp_strdup(l, rs));

	curl_free(rs);
fail:   curl_easy_cleanup(c);
	return ret;

}

static lisp_cell_t *subr_curl_version(lisp_t *l, lisp_cell_t *args)
{
	UNUSED(args);
	return mk_immutable_str(l, curl_version());
}

static lisp_cell_t *subr_curl_time(lisp_t *l, lisp_cell_t *args)
{
	time_t t = curl_getdate(get_str(car(args)), NULL);
	/**@warning there may be some trickery here according to the documentation
	 * https://curl.haxx.se/libcurl/c/curl_getdate.html */
	if(t == -1)
		return gsym_error();
	return mk_int(l, t);
}

int lisp_module_initialize(lisp_t *l)
{
	assert(l);

	pthread_mutex_lock(&curl_global_initialize_lock);

	if(locked_initialize_failed)
		goto unlock_fail;

	if(!locked_initialize_done) {
		if(curl_global_init(CURL_GLOBAL_ALL)) {
			locked_initialize_failed = 1;
			goto unlock_fail;
		}
		locked_initialize_done = 1;
	}
	pthread_mutex_unlock(&curl_global_initialize_lock);

	if(lisp_add_module_subroutines(l, primitives, 0))
		goto fail;
	return 0;
unlock_fail:
	pthread_mutex_unlock(&curl_global_initialize_lock);
 fail:	
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__ ((constructor));
static void destruct(void) __attribute__ ((destructor));
static void construct(void) { }
static void destruct(void) { }
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
