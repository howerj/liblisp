/** @file       liblisp_curl.c
 *  @brief      Interface to curl https://curl.haxx.se/libcurl/
 *  @author     Richard Howe (2015)
 *  @email      howe.r.j.89@gmail.com
 *  @todo       Port to Windows
 *
 * Code for this module mainly comes from https://curl.haxx.se examples
 * on Curl, as such:
 *
 * COPYRIGHT AND PERMISSION NOTICE
 *
 * Copyright (c) 1996 - 2016, Daniel Stenberg, <daniel@haxx.se>, and many
 * contributors, see the THANKS file.
 *
 * All rights reserved.
 *
 * Permission to use, copy, modify, and distribute this software for any purpose
 * with or without fee is hereby granted, provided that the above copyright
 * notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT OF THIRD PARTY RIGHTS. IN
 * NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE
 * OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Except as contained in this notice, the name of a copyright holder shall not
 * be used in advertising or otherwise to promote the sale, use or other dealings
 * in this Software without prior written authorization of the copyright holder.
 *
 **/
#include <lispmod.h>
#include <assert.h>
#include <curl/curl.h>
#include <time.h>

static lisp_mutex_t curl_global_initialize_lock = LISP_MUTEX_INITIALIZER;
static int locked_initialize_done = 0;
static int locked_initialize_failed = 0;

#define SUBROUTINE_XLIST\
	X("curl-get",     subr_curl_get, "Z Z b",  "download a URL to a file, with optional debugging")\
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
	{NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};

#undef X

static size_t write_data(void *ptr, size_t size, size_t nmemb, void *stream)
{
	return fwrite(ptr, size, nmemb, (FILE *) stream);
}

struct data {
	char trace_ascii;	/* 1 or 0 */
};

/* see https://curl.haxx.se/libcurl/c/debug.html */
static void dump(const char *text, FILE * stream, unsigned char *ptr, size_t size, char nohex)
{
	size_t i;
	size_t c;

	unsigned int width = 0x10;

	if (nohex)
		/* without the hex output, we can fit more on screen */
		width = 0x40;

	fprintf(stream, "%s, %10.10ld bytes (0x%8.8lx)\n", text, (long)size, (long)size);

	for (i = 0; i < size; i += width) {

		fprintf(stream, "%4.4lx: ", (long)i);

		if (!nohex) {
			/* hex not disabled, show it */
			for (c = 0; c < width; c++)
				if (i + c < size)
					fprintf(stream, "%02x ", ptr[i + c]);
				else
					fputs("   ", stream);
		}

		for (c = 0; (c < width) && (i + c < size); c++) {
			/* check for 0D0A; if found, skip past and start a new line of output */
			if (nohex && (i + c + 1 < size) && ptr[i + c] == 0x0D && ptr[i + c + 1] == 0x0A) {
				i += (c + 2 - width);
				break;
			}
			fprintf(stream, "%c", (ptr[i + c] >= 0x20)
				&& (ptr[i + c] < 0x80) ? ptr[i + c] : '.');
			/* check again for 0D0A, to avoid an extra \n if it's at width */
			if (nohex && (i + c + 2 < size)
			    && ptr[i + c + 1] == 0x0D && ptr[i + c + 2] == 0x0A) {
				i += (c + 3 - width);
				break;
			}
		}
		fputc('\n', stream);	/* newline */
	}
	fflush(stream);
}

static int my_trace(CURL * handle, curl_infotype type, char *data, size_t size, void *userp)
{
	struct data *config = (struct data *)userp;
	const char *text;
	UNUSED(handle);

	switch (type) {
	case CURLINFO_TEXT:
		fprintf(stderr, "== Info: %s", data);
	default:		/* in case a new one is introduced to shock us */
		return 0;

	case CURLINFO_HEADER_OUT:
		text = "=> Send header";
		break;
	case CURLINFO_DATA_OUT:
		text = "=> Send data";
		break;
	case CURLINFO_SSL_DATA_OUT:
		text = "=> Send SSL data";
		break;
	case CURLINFO_HEADER_IN:
		text = "<= Recv header";
		break;
	case CURLINFO_DATA_IN:
		text = "<= Recv data";
		break;
	case CURLINFO_SSL_DATA_IN:
		text = "<= Recv SSL data";
		break;
	}

	dump(text, stderr, (unsigned char *)data, size, config->trace_ascii);
	return 0;
}

static lisp_cell_t *subr_curl_get(lisp_t * l, lisp_cell_t * args)
{ /**@todo rewrite so it uses an IO port*/
	UNUSED(l);
	CURL *c;
	char *fileout = get_str(CADR(args));
	FILE *pagefile;
	lisp_cell_t *ret = gsym_error();
	struct data config;
	config.trace_ascii = 1;	/* enable ascii tracing */

	if (!(c = curl_easy_init()))
		return ret;

	curl_easy_setopt(c, CURLOPT_URL, get_str(car(args)));
	curl_easy_setopt(c, CURLOPT_WRITEFUNCTION, write_data);
	if ((pagefile = fopen(fileout, "wb"))) {
		if (CADDR(args) == gsym_tee()) {
			curl_easy_setopt(c, CURLOPT_DEBUGFUNCTION, my_trace);
			curl_easy_setopt(c, CURLOPT_DEBUGDATA, &config);
			curl_easy_setopt(c, CURLOPT_VERBOSE, 1L);
		}
		curl_easy_setopt(c, CURLOPT_WRITEDATA, pagefile);
		curl_easy_perform(c);
		fclose(pagefile);
		curl_easy_cleanup(c);
		ret = gsym_tee();
	}
	return ret;
}

static lisp_cell_t *subr_url_encode(lisp_t * l, lisp_cell_t * args)
{
	lisp_cell_t *ret = gsym_error();
	CURL *c = curl_easy_init();
	char *rs = NULL;
	if (!c)
		return gsym_error();
	if (!(rs = curl_easy_escape(c, get_sym(car(args)), get_length(car(args)))))
		goto fail;
	ret = mk_str(l, lisp_strdup(l, rs));

	curl_free(rs);
 fail:	curl_easy_cleanup(c);
	return ret;
}

static lisp_cell_t *subr_url_decode(lisp_t * l, lisp_cell_t * args)
{
	lisp_cell_t *ret = gsym_error();
	CURL *c = curl_easy_init();
	char *rs = NULL;
	if (!c)
		return gsym_error();
	/*although curl_easy_unescape can deal with strings with NUL in them,
	 *liblisp cannot at the moment*/
	if (!(rs = curl_easy_unescape(c, get_sym(car(args)), get_length(car(args)), NULL)))
		goto fail;
	ret = mk_str(l, lisp_strdup(l, rs));

	curl_free(rs);
 fail:	curl_easy_cleanup(c);
	return ret;

}

static lisp_cell_t *subr_curl_version(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_immutable_str(l, curl_version());
}

static lisp_cell_t *subr_curl_time(lisp_t * l, lisp_cell_t * args)
{
	time_t t = curl_getdate(get_str(car(args)), NULL);
	/**@warning there may be some trickery here according to the documentation
	 * https://curl.haxx.se/libcurl/c/curl_getdate.html */
	if (t == -1)
		return gsym_error();
	return mk_int(l, t);
}

int lisp_module_initialize(lisp_t * l)
{
	assert(l);

	lisp_mutex_lock(&curl_global_initialize_lock);

	if (locked_initialize_failed)
		goto unlock_fail;

	if (!locked_initialize_done) {
		if (curl_global_init(CURL_GLOBAL_ALL)) {
			locked_initialize_failed = 1;
			goto unlock_fail;
		}
		locked_initialize_done = 1;
	}
	lisp_mutex_unlock(&curl_global_initialize_lock);

	if (lisp_add_module_subroutines(l, primitives, 0))
		goto fail;
	return 0;
 unlock_fail:
	lisp_mutex_unlock(&curl_global_initialize_lock);
 fail:
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__ ((constructor));
static void destruct(void) __attribute__ ((destructor));
static void construct(void)
{
}

static void destruct(void)
{
	curl_global_cleanup();
}
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
