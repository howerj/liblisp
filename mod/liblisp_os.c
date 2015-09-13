/** @file       liblisp_os.c
 *  @brief      OS module interface module for liblisp
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com**/
#define _GNU_SOURCE
#include <assert.h>
#include <liblisp.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <fcntl.h>
#include <unistd.h>
#include <dirent.h>
#include <signal.h>
#include <errno.h>

#include <linux/kdev_t.h>

extern lisp *lglobal; /* from main.c */

#define SUBROUTINE_XLIST\
        X(subr_chdir,     "chdir")\
        X(subr_directory, "ls")\
        X(subr_kill,      "kill")\
        X(subr_link,      "link")\
        X(subr_nice,      "nice")\
        X(subr_pause,     "pause")\
        X(subr_sleep,     "sleep")\
        X(subr_symlink,   "symlink")\
        X(subr_sync,      "sync")\
        X(subr_rmdir,     "rmdir")\
        X(subr_chown,     "chown")\
        X(subr_chmod,     "chmod")\
        X(subr_mount,     "mount")\
        X(subr_umount,    "umount")\
        X(subr_mknod,     "mknod")\
        X(subr_ualarm,    "ualarm")

#define X(SUBR, NAME) static cell* SUBR (lisp *l, cell *args);
SUBROUTINE_XLIST /*function prototypes for all of the built-in subroutines*/
#undef X

#define X(SUBR, NAME) { SUBR, NAME },
static struct module_subroutines { subr p; char *name; } primitives[] = {
        SUBROUTINE_XLIST /*all of the subr functions*/
        {NULL, NULL} /*must be terminated with NULLs*/
};
#undef X

static void construct(void) __attribute__((constructor));
static void destruct(void) __attribute__((destructor));

static cell *subr_mknod(lisp *l, cell *args) {
        dev_t d;
        mode_t m = S_IFIFO;
        assert(l == lglobal);
        if(!cklen(args, 4) || !is_asciiz(car(args)) 
            || !is_asciiz(CADR(args)) || !is_int(CADDR(args)) || !is_int(CADDDR(args)))
                RECOVER(l, "\"(string string integer integer)\" '%S", args);
        if(!cklen(CADR(args), 1))
                goto invalid;
        switch(strval(CADR(args))[0]) {
                case 'c': /*fall through*/
                case 'u': m = S_IFCHR; break;
                case 'b': m = S_IFBLK; break;
                case 'p': m = S_IFIFO; break;
        invalid: default: RECOVER(l, "\"invalid node type (not 'c 'u 'b or 'p)\" %s", strval(CADR(args)));
        }
        d = MKDEV(intval(CADDR(args)), intval(CADDDR(args)));
        return mkint(l, mknod(strval(car(args)), m | S_IRWXU, d));
}

static cell *subr_chmod(lisp *l, cell *args) {
        assert(l == lglobal);
        if(!cklen(args, 2) || !is_asciiz(car(args)) || !is_int(CADR(args)))
                RECOVER(l, "\"(string integer)\" '%S", args);
        return mkint(l, chmod(strval(car(args)), intval(CADR(args))));
}

static cell *subr_mount(lisp *l, cell *args) {
        assert(l == lglobal);
        if(!cklen(args, 3) || !is_asciiz(car(args)) || !is_asciiz(CADR(args)) || !is_asciiz(CADDR(args)))
                RECOVER(l, "\"(string string string)\" '%S", args);
        return mkint(l, mount(strval(car(args)), strval(CADR(args)), strval(CADDR(args)), MS_MGC_VAL, NULL));
}

static cell *subr_umount(lisp *l, cell *args) {
        assert(l == lglobal);
        if(!cklen(args, 1) || !is_asciiz(car(args)))
                RECOVER(l, "\"(string)\" '%S", args);
        return mkint(l, umount(strval(car(args))));
}

static cell *subr_chown(lisp *l, cell *args) {
        assert(l == lglobal);
        if(!cklen(args, 3) || !is_asciiz(car(args)) || !is_int(CADR(args)) || !is_int(CADDR(args)))
                RECOVER(l, "\"expected () or (string int int)\" '%S", args);
        return mkint(l, chown(strval(car(args)), intval(CADR(args)), intval(CADDR(args))));
}

static cell* subr_directory(lisp *l, cell *args) {
        DIR *d;
        struct dirent *e;
        cell *ret = gsym_nil();
        char *s = NULL;
        assert(l == lglobal);
        if(cklen(args, 0)) s = ".";
        else if(!cklen(args, 1) || !is_asciiz(car(args)))
                RECOVER(l, "\"expected () or (string)\" '%S", args);
        s = s ? s : strval(car(args));
        if(!(d = opendir(s)))
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

static cell *subr_nice(lisp *l, cell *args) {
        if(!cklen(args, 1) || !is_int(car(args)))
                RECOVER(l, "\"expected (integer)\" '%S", args);
        return mkint(l, nice(intval(car(args))));
}

static cell *subr_pause(lisp *l, cell *args) {
        if(!cklen(args, 0))
                RECOVER(l, "\"expected ()\" '%S", args);
        return mkint(l, pause());
}

static cell *subr_symlink(lisp *l, cell *args) {
        if(!cklen(args, 2) || !is_asciiz(car(args)) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (string string)\" '%S", args);
        return mkint(l, symlink(strval(car(args)), strval(CADR(args))));
}

static cell *subr_link(lisp *l, cell *args) {
        if(!cklen(args, 2) || !is_asciiz(car(args)) || !is_asciiz(CADR(args)))
                RECOVER(l, "\"expected (string string)\" '%S", args);
        return mkint(l, link(strval(car(args)), strval(CADR(args))));
}

static cell *subr_chdir(lisp *l, cell *args) {
        if(!cklen(args, 1) || !is_asciiz(car(args)))
                RECOVER(l, "\"expected (string)\" '%S", args);
        return mkint(l, chdir(strval(car(args))));
}

static cell *subr_ualarm(lisp *l, cell *args) {
        if(!cklen(args, 2) || !is_int(car(args)) || !is_int(CADR(args)))
                RECOVER(l, "\"expected (integer integer)\" '%S", args);
        return mkint(l, ualarm(intval(car(args)), intval(CADR(args))));
}

static cell *subr_rmdir(lisp *l, cell *args) {
        if(!cklen(args, 1) || !is_asciiz(car(args)))
                RECOVER(l, "\"expected (string)\" '%S", args);
        return mkint(l, rmdir(strval(car(args))));
}

static void construct(void) {
        size_t i;
        assert(lglobal);
        for(i = 0; primitives[i].p; i++) /*add all primitives from this module*/
                if(!lisp_add_subr(lglobal, primitives[i].name, primitives[i].p))
                        goto fail;
        printerf(lglobal, lisp_get_output(lglobal), 0, "module: OS loaded\n");
        return;
fail:   printerf(lglobal, lisp_get_output(lglobal), 0, "module: OS loading failure\n");
}

static void destruct(void) {
}
