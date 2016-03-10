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
 *
 *      int          access(const char *, int);
 *      int          brk(void *);
 *      int          chroot(const char *); (LEGACY)
 *      size_t       confstr(int, char *, size_t);
 *      char        *crypt(const char *, const char *);
 *      char        *ctermid(char *);
 *      char        *cuserid(char *s); (LEGACY)
 *      void         encrypt(char[64], int);
 *      int          execl(const char *, const char *, ...);
 *      int          execle(const char *, const char *, ...);
 *      int          execlp(const char *, const char *, ...);
 *      int          execv(const char *, char *const []);
 *      int          execve(const char *, char *const [], char *const []);
 *      int          execvp(const char *, char *const []);
 *      int          fchown(int, uid_t, gid_t);
 *      long         fpathconf(int, int);
 *      int          ftruncate(int, off_t);
 *      char        *getcwd(char *, size_t);
 *      int          getdtablesize(void); (LEGACY)
 *      int          getgroups(int, gid_t []);
 *      long         gethostid(void);
 *      int          getlogin_r(char *, size_t);
 *      int          getopt(int, char * const [], const char *);
 *      int          getpagesize(void); (LEGACY)
 *      char        *getpass(const char *); (LEGACY)
 *      char        *getwd(char *);
 *      int          lchown(const char *, uid_t, gid_t);
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
 *      int          setpgid(pid_t, pid_t);
 *      int          setregid(gid_t, gid_t);
 *      int          setreuid(uid_t, uid_t);
 *      pid_t        setpgrp(void);
 *      pid_t        setsid(void);
 *      int          setuid(uid_t);
 * >    void         swab(const void *, void *, ssize_t);
 *      long         sysconf(int);
 *      pid_t        tcgetpgrp(int);
 *      int          tcsetpgrp(int, pid_t);
 *      int          truncate(const char *, off_t);
 *      int          ttyname_r(int, char *, size_t);
 *      ssize_t      write(int, const void *, size_t); 
 *
 *      int open(const char *path, int oflag, ... );
 *
 *
 **/

#define SUBROUTINE_XLIST\
	X("ls",          subr_directory, "Z",       "list a directory contents")\
	X("stat",        subr_stat,      "Z",      "display file status")\
	X("_chdir",      subr_chdir,     "Z",       "change the current directory")\
	X("_kill",       subr_kill,      "d d",     "send a signal to a process")\
	X("_link",       subr_link,      "Z Z",     "make a hard link")\
	X("_nice",       subr_nice,      "d",       "set the niceness level of a process")\
	X("_pause",      subr_pause,     "",        "pause until a signal arrives")\
	X("_sleep",      subr_sleep,     "d",       "sleep for an amount of time")\
	X("_symlink",    subr_symlink,   "Z Z",     "create a symbolic link")\
	X("_sync",       subr_sync,      "",        "flush the file system buffers")\
	X("_rmdir",      subr_rmdir,     "Z",       "remove a directory")\
	X("_chown",      subr_chown,     "Z d d",   "change the ownership settings of a file")\
	X("_chmod",      subr_chmod,     "Z d",     "change the permissions of a file")\
	X("_mount",      subr_mount,     "Z Z Z",   "mount a file system")\
	X("_umount",     subr_umount,    "Z",       "unmount a file system")\
	X("_mknod",      subr_mknod,     "Z Z d d", "make a device node")\
	X("_ualarm",     subr_ualarm,    "d d",     "send SIGALARM to calling process after a time")\
	X("_getpgid",    subr_getpgid,   "d",       "get the process group id for a process")\
	X("_getuid",     subr_getuid,    "",        "get the real user id")\
	X("_getppid",    subr_getppid,   "",        "get the parent process id")\
	X("_getpid",     subr_getpid,    "",        "get the process id")\
	X("_getpgrp",    subr_getpgrp,   "",        "get process id of calling process")\
	X("_getsid",     subr_getsid,    "d",       "get the process group id a session leader")\
	X("_isatty",     subr_isatty,    "d",       "is file descriptor a tty?")\
	X("_dup",        subr_dup,       "d",       "duplicate open file descriptor")\
	X("_alarm",      subr_alarm,     "d",       "schedule an alarm signal to be generated")\
	X("_close",      subr__close,    "d",       "close a file descriptor")\
	X("_exit",       subr_exit,      "d",       "exit the process")\
	X("_fchdir",     subr_fchdir,    "d",       "change working directory given file descriptor")\
	X("_fsync",      subr_fsync,     "d",       "synchronize changes to file descriptor")\
	X("_setgid",     subr_setgid,    "d",       "set group id")\
	X("_fdatasync",  subr_fdatasync, "d",       "synchronize data to file descriptor")\
	X("_ulink",      subr_ulink,     "Z",       "remove directory entry")\
	X("_ttyname",    subr_ttyname,   "d",       "find pathname for a terminal")\
	X("_dup2",       subr_dup2,      "d d",     "duplicate a file descriptor into second descriptor handle")\
	X("_usleep",     subr_usleep,    "d",       "sleep for useconds")\
	X("_getegid",    subr_getegid,   "",        "get effective group id")\
	X("_geteuid",    subr_geteuid,   "",        "get effective user id")\
	X("_getgid",     subr_getgid,    "",        "get group id")\
	X("_fork",       subr_fork,      "",        "fork the process")\
	X("_vfork",      subr_vfork,     "",        "fork the process with shared virtual memory")\
	X("_getlogin",   subr_getlogin,  "",        "get login name")


#define X(NAME, SUBR, VALIDATION, DOCSTRING) static lisp_cell_t * SUBR (lisp_t *l, lisp_cell_t *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(NAME, DOCSTRING), SUBR },
static lisp_module_subroutines_t primitives[] = {
	SUBROUTINE_XLIST	/*all of the subr functions */
	{ NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};

#undef X

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


static lisp_cell_t *subr_stat(lisp_t * l, lisp_cell_t * args)
{
	lisp_cell_t *type;
	int r = 0;
	struct stat st;
	if((r = stat(get_str(car(args)), &st)) < 0)
		return cons(l, gsym_error(), mk_int(l, r));
	switch(st.st_mode & S_IFMT){
        case S_IFBLK:  type = mk_immutable_str(l, "block"); break;
        case S_IFCHR:  type = mk_immutable_str(l, "character"); break;
        case S_IFSOCK: type = mk_immutable_str(l, "socket"); break;
        case S_IFIFO:  type = mk_immutable_str(l, "fifo"); break;
        case S_IFDIR:  type = mk_immutable_str(l, "directory"); break;
        case S_IFREG:  type = mk_immutable_str(l, "regular"); break;
        case S_IFLNK:  type = mk_immutable_str(l, "symlink"); break;
        default:       type = mk_immutable_str(l, "unknown"); break;
        }

	return mk_list(l, type,
		mk_int(l, major(st.st_dev)),
		mk_int(l, minor(st.st_dev)),
		mk_int(l, st.st_size),
		mk_int(l, st.st_mode),
		mk_int(l, st.st_ino),
		mk_int(l, st.st_nlink),
		mk_int(l, st.st_uid),
		mk_int(l, st.st_gid),
		mk_int(l, st.st_blksize),
		mk_int(l, st.st_blocks),
		mk_int(l, st.st_ctime),
		mk_int(l, st.st_atime),
		mk_int(l, st.st_mtime), 
		NULL);
}

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

static lisp_cell_t *subr_getpgid(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, getpgid(get_int(car(args))));
}

static lisp_cell_t *subr_getpgrp(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_int(l, getpgrp());
}

static lisp_cell_t *subr_getpid(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_int(l, getpid());
}
static lisp_cell_t *subr_getppid(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_int(l, getppid());
}

static lisp_cell_t *subr_getsid(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, getsid(get_int(car(args))));
}

static lisp_cell_t *subr_getuid(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_int(l, getuid());
}

static lisp_cell_t *subr_isatty(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	return mk_int(l, isatty(get_int(car(args))));
}

static lisp_cell_t *subr_dup(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, dup(get_int(car(args))));
}

static lisp_cell_t *subr_alarm(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, alarm(get_int(car(args))));
}

static lisp_cell_t *subr__close(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, close(get_int(car(args))));
}

static lisp_cell_t *subr_exit(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(l);
	_exit(get_int(car(args)));
	return gsym_error(); /*unreachable, hopefully*/
}

static lisp_cell_t *subr_fchdir(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, fchdir(get_int(car(args))));
}

static lisp_cell_t *subr_fsync(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, fsync(get_int(car(args))));
}

static lisp_cell_t *subr_setgid(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, setgid(get_int(car(args))));
}

static lisp_cell_t *subr_fdatasync(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, fdatasync(get_int(car(args))));
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

static lisp_cell_t *subr_ulink(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, unlink(get_str(car(args))));
}

static lisp_cell_t *subr_ttyname(lisp_t * l, lisp_cell_t * args)
{
	char *s;
	return mk_str(l, lisp_strdup(l, (s = ttyname(get_int(car(args)))) ? s : ""));
}

static lisp_cell_t *subr_dup2(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, dup2(get_int(car(args)), get_int(CADR(args))));
}

static lisp_cell_t *subr_usleep(lisp_t * l, lisp_cell_t * args)
{
	return mk_int(l, usleep(get_int(car(args))));
}

static lisp_cell_t *subr_getegid(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_int(l, getegid());
}

static lisp_cell_t *subr_geteuid(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_int(l, geteuid());
}

static lisp_cell_t *subr_getgid(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_int(l, getgid());
}

static lisp_cell_t *subr_fork(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_int(l, fork());
}

static lisp_cell_t *subr_vfork(lisp_t * l, lisp_cell_t * args)
{
	UNUSED(args);
	return mk_int(l, vfork());
}

static lisp_cell_t *subr_getlogin(lisp_t * l, lisp_cell_t * args)
{
	char *s;
	UNUSED(args);
	return mk_immutable_str(l, (s = getlogin()) ? s : "");
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
