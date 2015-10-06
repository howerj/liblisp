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

extern lisp *lglobal; /* from main.c */

static void construct(void) __attribute__((constructor));
static void destruct(void) __attribute__((destructor));

static cell* subr_diff(lisp *l, cell *args) {
        UNUSED(l); UNUSED(args);
        return gsym_nil();
}

static void construct(void) {
        assert(lglobal);
        if(!(lisp_add_subr(lglobal, "diff", subr_diff))) goto fail;
        lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: diff loaded\n");
        return;
fail:   lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: diff load failure\n");
}

static void destruct(void) {
}

