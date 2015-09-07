#include <liblisp.h>
cell* subr_example(lisp *l, cell *args) {
        if(!cklen(args, 1))
                RECOVER(l, "\"expected (int)\" '%S", args);
        return mkint(l, intval(car(args)) * 2);
}

