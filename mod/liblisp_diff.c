/** @file       liblisp_diff.c
 *  @brief      diff util module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *
 *  @bug implement me!**/

#include "diff.h"
#include <assert.h>
#include <liblisp.h>

static cell* subr_diff(lisp *l, cell *args) {
        UNUSED(l); UNUSED(args);
        return gsym_nil();
}

static int initialize(void) {
        assert(lglobal);
        if(!(lisp_add_subr(lglobal, "diff", subr_diff))) goto fail;
        lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: diff loaded\n");
        return 0;
fail:   lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: diff load failure\n");
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__((constructor));
static void destruct(void)  __attribute__((destructor));
static void construct(void) { initialize(); }
static void destruct(void)  { }
#elif _WIN32
#include <windows.h>
BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD fdwReason, LPVOID lpvReserved) {
        switch (fdwReason) {
            case DLL_PROCESS_ATTACH: initialize(); break;
            case DLL_PROCESS_DETACH: break;
            case DLL_THREAD_ATTACH:  break;
            case DLL_THREAD_DETACH:  break;
	    default: break;
        }
        return TRUE;
}
#endif

