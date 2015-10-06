/** @file       liblisp_tsort.c
 *  @brief      Topological sort module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *
 *  @todo implement me!**/

#include "tsort.h"
#include <assert.h>
#include <liblisp.h>

extern lisp *lglobal; /* from main.c */

static void construct(void) __attribute__((constructor));
static void destruct(void) __attribute__((destructor));

static cell* subr_tsort(lisp *l, cell *args) {
        UNUSED(l); UNUSED(args);
        return gsym_nil();
}

static void construct(void) {
        assert(lglobal);
        if(!(lisp_add_subr(lglobal, "", subr_tsort))) goto fail;
        lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: tsort loaded\n");
        return;
fail:   lisp_printf(lglobal, lisp_get_logging(lglobal), 0, "module: tsort load failure\n");
}

static void destruct(void) {
}

