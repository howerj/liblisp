#define _POSIX_SOURCE
#include <assert.h>
#include <liblisp.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>

extern lisp *lglobal; /* from main.c */

static void construct(void) __attribute__((constructor));
static void destruct(void) __attribute__((destructor));

static cell* subr_directory(lisp *l, cell *args) {
        DIR *d;
        struct dirent *e;
        cell *ret = gsym_nil();
        assert(l == lglobal);
        if(!cklen(args, 1) || !is_asciiz(car(args)))
                RECOVER(l, "\"expected (string)\" '%S", args);
        if(!(d = opendir(strval(car(args)))))
                return gsym_error();
        while((e = readdir(d)))
                ret = cons(l, mkstr(lglobal, lstrdup(e->d_name)), ret);
        closedir(d);
        return ret;
}

static cell* subr_sleep(lisp *l, cell *args) {
        if(!cklen(args, 1) || !is_int(car(args)))
                RECOVER(l, "\"expected (integer)\" '%S", args);
        return mkint(l, sleep(intval(car(args))));
}

static cell *subr_sync(lisp *l, cell *args) {
        if(!cklen(args, 0))
                RECOVER(l, "\"expected ()\" '%S", args);
        sync();
        return gsym_tee();
}

static cell *subr_kill(lisp *l, cell *args) {
        if(!cklen(args, 2) || !is_int(car(args)) || !is_int(CADR(args)))
                RECOVER(l, "\"expected (integer integer)\" '%S", args);
        return mkint(l, kill(intval(car(args)), intval(CADR(args))));
}

static void construct(void) {
        lisp_add_subr(lglobal, "list-directory", subr_directory);
        lisp_add_subr(lglobal, "sleep", subr_sleep);
        lisp_add_subr(lglobal, "sync",  subr_sync);
        lisp_add_subr(lglobal, "kill",  subr_kill);
}

static void destruct(void) {
}
