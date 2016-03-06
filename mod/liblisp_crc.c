/** @file       liblisp_crc.c
 *  @brief      CRC module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com **/

#include "crc.h"
#include <assert.h>
#include <liblisp.h>

static cell *subr_crc(lisp * l, cell * args)
{
	UNUSED(l);
	uint32_t c;
	c = crc_final(crc_init((uint8_t *) get_str(car(args)), get_length(car(args))));
	return mk_int(l, c);
}

int lisp_module_initialize(lisp *l)
{
	assert(l);
	if (!(lisp_add_subr(l, "crc", subr_crc, "Z", "CRC-32 of a string")))
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
