/** @file       lispmod.h
 *  @brief      liblisp module interface and utility functions
 *  @author     Richard Howe (2016)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com */
 
#ifndef LISPMOD_H
#define LISPMOD_H

#include <liblisp.h>

#ifdef __unix__
#include <dlfcn.h>
#include <pthread.h>
#elif __WIN32
#include <windows.h>
#endif

/* mutex */
#ifdef __unix__
typedef pthread_mutex_t lisp_mutex_t;
#define LISP_MUTEX_INITIALIZER PTHREAD_MUTEX_INITIALIZER
#elif __WIN32
typedef LPCRITICAL_SECTION lisp_mutex_t;
#define LISP_MUTEX_INITIALIZER NULL /*@todo handle this on Windows*/
#endif

lisp_mutex_t* lisp_mutex_create(void);
int lisp_mutex_lock(lisp_mutex_t *m);
int lisp_mutex_trylock(lisp_mutex_t *m);
int lisp_mutex_unlock(lisp_mutex_t *m);
/********/

#ifdef __unix__
typedef void* dl_handle_t;

#define DL_OPEN(NAME)        dlopen((NAME), RTLD_NOW)
#define DL_CLOSE(HANDLE)     dlclose((HANDLE))
#define DL_SYM(HANDLE, NAME) dlsym((HANDLE), (NAME))
#define DL_ERROR()           lisp_mod_dlerror()

const char *lisp_mod_dlerror(void);
#elif __WIN32
typedef HMODULE dl_handle_t;

#define DL_OPEN(NAME)        LoadLibrary((NAME))
#define DL_CLOSE(HANDLE)     FreeLibrary((HANDLE))
#define DL_SYM(HANDLE, NAME) (lisp_subr_func)GetProcAddress((HMODULE)(HANDLE), (NAME))
#define DL_ERROR()           lisp_mod_dlerror()

const char *lisp_mod_dlerror(void);
#endif 

#endif /* LISPMOD_H */
