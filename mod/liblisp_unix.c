/** @file       liblisp_unix.c
 *  @brief      Unix module interface module for liblisp
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *
 *  This is an Unix OS interface module. 
 *  @todo This module needs *significant* improvement **/
#define _GNU_SOURCE
#include <assert.h>
#include <liblisp.h>
#include <stdlib.h>

#ifdef __linux__
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
#elif _WIN32
#error "Windows is not supported"
#else
#error "Unsupported (Unix?) system"
#endif

/*      From <http://pubs.opengroup.org/onlinepubs/7908799/xsh/unistd.h.html>
 *      To implement (where appropriate):
 *      int          access(const char *, int);
 *      unsigned     alarm(unsigned int);
 *      int          brk(void *);
 *      int          chroot(const char *); (LEGACY)
 *      int          close(int);
 *      size_t       confstr(int, char *, size_t);
 *      char        *crypt(const char *, const char *);
 *      char        *ctermid(char *);
 *      char        *cuserid(char *s); (LEGACY)
 *      int          dup(int);
 *      int          dup2(int, int);
 *      void         encrypt(char[64], int);
 *      int          execl(const char *, const char *, ...);
 *      int          execle(const char *, const char *, ...);
 *      int          execlp(const char *, const char *, ...);
 *      int          execv(const char *, char *const []);
 *      int          execve(const char *, char *const [], char *const []);
 *      int          execvp(const char *, char *const []);
 *      void        _exit(int);
 *      int          fchown(int, uid_t, gid_t);
 *      int          fchdir(int);
 *      int          fdatasync(int);
 *      pid_t        fork(void);
 *      long         fpathconf(int, int);
 *      int          fsync(int);
 *      int          ftruncate(int, off_t);
 *      char        *getcwd(char *, size_t);
 *      int          getdtablesize(void); (LEGACY)
 *      gid_t        getegid(void);
 *      uid_t        geteuid(void);
 *      gid_t        getgid(void);
 *      int          getgroups(int, gid_t []);
 *      long         gethostid(void);
 *      char        *getlogin(void);
 *      int          getlogin_r(char *, size_t);
 *      int          getopt(int, char * const [], const char *);
 *      int          getpagesize(void); (LEGACY)
 *      char        *getpass(const char *); (LEGACY)
 *      pid_t        getpgid(pid_t);
 *      pid_t        getpgrp(void);
 *      pid_t        getpid(void);
 *      pid_t        getppid(void);
 *      pid_t        getsid(pid_t);
 *      uid_t        getuid(void);
 *      char        *getwd(char *);
 *      int          isatty(int);
 *      int          lchown(const char *, uid_t, gid_t);
 *      int          link(const char *, const char *);
 *      int          lockf(int, int, off_t);
 *      off_t        lseek(int, off_t, int);
 *      long         pathconf(const char *, int);
 *      int          pipe(int [2]);
 *      ssize_t      pread(int, void *, size_t, off_t);
 *      int          pthread_atfork(void (*)(void), void (*)(void),void(*)(void));
 *      ssize_t      pwrite(int, const void *, size_t, off_t);
 *      ssize_t      read(int, void *, size_t);
 *      int          readlink(const char *, char *, size_t);
 *      void        *sbrk(intptr_t);
 *      int          setgid(gid_t);
 *      int          setpgid(pid_t, pid_t);
 *      pid_t        setpgrp(void);
 *      int          setregid(gid_t, gid_t);
 *      int          setreuid(uid_t, uid_t);
 *      pid_t        setsid(void);
 *      int          setuid(uid_t);
 * >    void         swab(const void *, void *, ssize_t);
 *      long         sysconf(int);
 *      pid_t        tcgetpgrp(int);
 *      int          tcsetpgrp(int, pid_t);
 *      int          truncate(const char *, off_t);
 *      char        *ttyname(int);
 *      int          ttyname_r(int, char *, size_t);
 *      int          unlink(const char *);
 *      int          usleep(useconds_t);
 *      pid_t        vfork(void);
 *      ssize_t      write(int, const void *, size_t); **/

#define SUBROUTINE_XLIST\
	X("chdir",      subr_chdir,     "Z",       "change the current directory")\
	X("ls",         subr_directory, "Z",       "list a directory contents")\
	X("kill",       subr_kill,      "d d",     "send a signal to a process")\
	X("link",       subr_link,      "Z Z",     "make a hard link")\
	X("nice",       subr_nice,      "d",       "set the niceness level of a process")\
	X("pause",      subr_pause,     "",        "pause until a signal arrives")\
	X("sleep",      subr_sleep,     "d",       "sleep for an amount of time")\
	X("symlink",    subr_symlink,   "Z Z",     "create a symbolic link")\
	X("sync",       subr_sync,      "",        "flush the file system buffers")\
	X("rmdir",      subr_rmdir,     "Z",       "remove a directory")\
	X("chown",      subr_chown,     "Z d d",   "change the ownership settings of a file")\
	X("chmod",      subr_chmod,     "Z d",     "change the permissions of a file")\
	X("mount",      subr_mount,     "Z Z Z",   "mount a file system")\
	X("umount",     subr_umount,    "Z",       "unmount a file system")\
	X("mknod",      subr_mknod,     "Z Z d d", "make a device node")\
	X("ualarm",     subr_ualarm,    "d d",     "send SIGALARM to calling process after a time")

#define X(NAME, SUBR, VALIDATION, DOCSTRING) static lisp_cell_t * SUBR (lisp_t *l, lisp_cell_t *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(NAME, DOCSTRING), SUBR },
static lisp_module_subroutines_t primitives[] = {
	SUBROUTINE_XLIST	/*all of the subr functions */
	{ NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};

#undef X

static lisp_cell_t *subr_mknod(lisp_t * l, lisp_cell_t * args)
{
	dev_t d;
	mode_t m = S_IFIFO;
	if (!lisp_check_length(CADR(args), 1))
		goto invalid;
	switch (get_str(CADR(args))[0]) {
	case 'c':		/*fall through */
	case 'u':
		m = S_IFCHR;
		break;
	case 'b':
		m = S_IFBLK;
		break;
	case 'p':
		m = S_IFIFO;
		break;
	invalid: default:
		LISP_RECOVER(l, "\"invalid node type (not 'c 'u 'b or 'p)\" %s", get_str(CADR(args)));
	}
	d = MKDEV(get_int(CADDR(args)), get_int(CADDDR(args)));
	return mk_int(l, mknod(get_str(car(args)), m | S_IRWXU, d));
}

static lisp_cell_t *subr_chmod(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, chmod(get_str(car(args)), get_int(CADR(args))));
}

static lisp_cell_t *subr_mount(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, mount(get_str(car(args)), get_str(CADR(args)), get_str(CADDR(args)), MS_MGC_VAL, NULL));
}

static lisp_cell_t *subr_umount(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, umount(get_str(car(args))));
}

static lisp_cell_t *subr_chown(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, chown(get_str(car(args)), get_int(CADR(args)), get_int(CADDR(args))));
}

static lisp_cell_t *subr_directory(lisp_t * l, lisp_cell_t * args)
{
	DIR *d;
	struct dirent *e;
	lisp_cell_t *ret = gsym_nil();
	if (!(d = opendir(get_str(car(args)))))
		return gsym_error();
	while ((e = readdir(d)))
		ret = cons(l, mk_str(l, lisp_strdup(l, e->d_name)), ret);
	closedir(d);
	return ret;
}

static lisp_cell_t *subr_sleep(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, sleep(get_int(car(args))));
}

static lisp_cell_t *subr_sync(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	UNUSED(args);
	sync();
	return gsym_tee();
}

static lisp_cell_t *subr_kill(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, kill(get_int(car(args)), get_int(CADR(args))));
}

static lisp_cell_t *subr_nice(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, nice(get_int(car(args))));
}

static lisp_cell_t *subr_pause(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_int(l, pause());
}

static lisp_cell_t *subr_symlink(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, symlink(get_str(car(args)), get_str(CADR(args))));
}

static lisp_cell_t *subr_link(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, link(get_str(car(args)), get_str(CADR(args))));
}

static lisp_cell_t *subr_chdir(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, chdir(get_str(car(args))));
}

static lisp_cell_t *subr_ualarm(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, ualarm(get_int(car(args)), get_int(CADR(args))));
}

static lisp_cell_t *subr_rmdir(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, rmdir(get_str(car(args))));
}

int lisp_module_initialize(lisp_t *l)
{
	assert(l);
	if(lisp_add_module_subroutines(l, primitives, 0) < 0)
		goto fail;
	return 0;
 fail:	
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__ ((constructor));
static void destruct(void) __attribute__ ((destructor));
static void construct(void) { }
static void destruct(void) { }
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
