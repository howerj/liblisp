/** @file       liblisp_crc.c
 *  @brief      CRC module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *  
 *  A small CRC module; using the following two links it will be the first
 *  module that an attempted port to the Windows operating system will be
 *  done under from Unix (Linux).
 * 
 * See <https://stackoverflow.com/questions/15454968/dll-plugin-that-uses-functions-defined-in-the-main-executable>
 * and <https://stackoverflow.com/questions/8863193/what-does-declspecdllimport-really-mean>
 **/

#include "crc.h"
#include <assert.h>
#include "liblisp.h"

static void construct(void) __attribute__((constructor));
static void destruct(void) __attribute__((destructor));

static cell* subr_crc(lisp *l, cell *args) {
        uint32_t c;
        if(!cklen(args, 1) || !is_asciiz(car(args)))
                RECOVER(l, "\"expected (string)\" '%S", args);
        c = crc_final(crc_init((uint8_t*)strval(car(args)), lisp_get_cell_length(car(args))));
        return mk_int(lglobal, c);
}

static int initialize(void) {
        assert(lglobal);
        if(!(lisp_add_subr(lglobal, "crc", subr_crc))) goto fail;
        lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: crc loaded\n");
        return 0;
fail:   lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: crc load failure\n");
        return -1;
}

#ifdef __unix__
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

