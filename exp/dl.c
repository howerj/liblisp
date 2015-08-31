#include "liblisp.h"
/*test for module system*/
cell *subr_dltst(lisp *l, cell *args) {
        (void)l;
        (void)args;
        return mkint(l, 99);
}
