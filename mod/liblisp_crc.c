#include "crc.h"
#include <assert.h>
#include <liblisp.h>

extern lisp *lglobal; /* from main.c */

static void construct(void) __attribute__((constructor));
static void destruct(void) __attribute__((destructor));

static cell* subr_crc(lisp *l, cell *args) {
        uint32_t c;
        if(!cklen(args, 1) || !is_asciiz(car(args)))
                RECOVER(l, "\"expected (string)\" '%S", args);
        c = crc_final(crc_init((uint8_t*)strval(car(args)), lisp_get_cell_length(car(args))));
        return mkint(lglobal, c);
}

static void construct(void) {
        if(!(lisp_add_subr(lglobal, "crc", subr_crc))) goto fail;
        printerf(lglobal, lisp_get_logging(lglobal), 0, "crc module loaded successfully\n");
        return;
fail:   printerf(lglobal, lisp_get_logging(lglobal), 0, "crc module loading failed\n");
}

static void destruct(void) {
}
