/** @file       liblisp_tcc.c
 *  @brief      Tiny C Compiler liblisp module
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later 
 *              <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email      howe.r.j.89@gmail.com
 *  @todo       I should find out if this module is thread safe or not
 **/
#include <assert.h>
#include <libtcc.h>
#include <liblisp.h>
#include <stdlib.h>
#include <stdint.h>

#define SUBROUTINE_XLIST\
        X("cc",                    subr_compile,      NULL, "compile a string as C code")\
        X("cc-link-library",       subr_link,         NULL, "link a library")\
        X("cc-file",               subr_compile_file, NULL, "compile a file")\
        X("cc-get-subroutine",     subr_get_subr,     NULL, "get a subroutine from a compilation")\
        X("cc-add-include-path",   subr_add_include_path, NULL, "add an include path for the C compiler")\
        X("cc-add-system-include-path", subr_add_sysinclude_path, NULL, "add a system include path for the C compiler")\
        X("cc-set-library-path",   subr_set_lib_path, NULL, "add a library path for the C compiler to look in")\

#define X(NAME, SUBR, VALIDATION, DOCSTRING) static cell* SUBR (lisp *l, cell *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(#SUBR, DOCSTRING), SUBR },
static struct module_subroutines {
	char *name, *validate, *docstring;
	subr p;
} primitives[] = {
	SUBROUTINE_XLIST	/*all of the subr functions */
	{NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};

#undef X

static int ud_tcc = 0;

static void ud_tcc_free(cell * f)
{
	tcc_delete(get_user(f));
	free(f);
}

static int ud_tcc_print(io * o, unsigned depth, cell * f)
{
	return lisp_printf(NULL, o, depth, "%B<COMPILE-STATE:%d>%t", get_user(f));
}

static cell *subr_compile(lisp * l, cell * args)
{
	if (!cklen(args, 3)
	    || !is_usertype(car(args), ud_tcc)
	    || !is_asciiz(CADR(args)) || !is_str(CADDR(args)))
		RECOVER(l, "\"expected (compile-state string string\" '%S", args);
	char *fname = get_str(CADR(args)), *prog = get_str(CADDR(args));
	subr func;
	TCCState *st = get_user(car(args));
	if (tcc_compile_string(st, prog) < 0)
		return gsym_error();
	if (tcc_relocate(st, TCC_RELOCATE_AUTO) < 0)
		return gsym_error();
	func = (subr) tcc_get_symbol(st, fname);
	return mk_subr(l, func, NULL, NULL);
}

static cell *subr_link(lisp * l, cell * args)
{
	if (!cklen(args, 2)
	    || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
		RECOVER(l, "\"expected (compile-state string)\" '%S", args);
	return tcc_add_library(get_user(car(args)), get_str(CADR(args))) < 0 ? gsym_error() : gsym_nil();
}

static cell *subr_compile_file(lisp * l, cell * args)
{
	if (!cklen(args, 2)
	    || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
		RECOVER(l, "\"expected (compile-state string)\" '%S", args);
	if (tcc_add_file(get_user(car(args)), get_str(CADR(args))) < 0)
		return gsym_error();
	if (tcc_relocate(get_user(car(args)), TCC_RELOCATE_AUTO) < 0)
		return gsym_error();
	return gsym_tee();
}

static cell *subr_get_subr(lisp * l, cell * args)
{
	subr func;
	if (!cklen(args, 2)
	    || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
		RECOVER(l, "\"expected (compile-state string)\" '%S", args);
	if (!(func = tcc_get_symbol(get_user(car(args)), get_str(CADR(args)))))
		return gsym_error();
	else
		return mk_subr(l, func, NULL, NULL);
}

static cell *subr_add_include_path(lisp * l, cell * args)
{
	if (!cklen(args, 2) || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
		RECOVER(l, "\"expected (compile-state string)\" '%S", args);
	return tcc_add_include_path(get_user(car(args)), get_str(CADR(args))) < 0 ? gsym_error() : gsym_tee();
}

static cell *subr_add_sysinclude_path(lisp * l, cell * args)
{
	if (!cklen(args, 2) || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
		RECOVER(l, "\"expected (compile-state string)\" '%S", args);
	return tcc_add_sysinclude_path(get_user(car(args)), get_str(CADR(args))) < 0 ? gsym_error() : gsym_tee();
}

static cell *subr_set_lib_path(lisp * l, cell * args)
{
	if (!cklen(args, 2) || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
		RECOVER(l, "\"expected (compile-state string)\" '%S", args);
	tcc_set_lib_path(get_user(car(args)), get_str(CADR(args)));
	return gsym_tee();
}

int lisp_module_initialize(lisp *l)
{
	size_t i;
	assert(l);
	/** Tiny C compiler library interface, special care has to be taken 
         *  when compiling and linking all of the C files within the liblisp
         *  project so the symbols in it are available to libtcc.
         *
         *  Possible improvements:
         *  * Modification of libtcc so it can accept S-Expressions from
         *    the interpreter. This would be a significant undertaking.
         *  * Add more functions from libtcc
         *  * Separate out tcc_get_symbol from tcc_compile_string
         *  * Find out why link does not work
         **/
	/**@bug ud_tcc needs to be on a per lisp thread basis*/
	ud_tcc = new_user_defined_type(l, ud_tcc_free, NULL, NULL, ud_tcc_print);
	if (ud_tcc < 0)
		goto fail;
	TCCState *st = tcc_new();
	tcc_set_output_type(st, TCC_OUTPUT_MEMORY);
	lisp_add_cell(l, "*compile-state*", mk_user(l, st, ud_tcc));
	for (i = 0; primitives[i].p; i++)	/*add all primitives from this module */
		if (!lisp_add_subr(l, primitives[i].name, primitives[i].p, primitives[i].validate, primitives[i].docstring))
			goto fail;
	if (lisp_verbose_modules)
		lisp_printf(l, lisp_get_logging(l), 0, "module: tcc loaded\n");
	return 0;
 fail:	lisp_printf(l, lisp_get_logging(l), 0, "module: tcc load failure\n");
	return -1;
}

#ifdef __unix__
static void construct(void) __attribute__ ((constructor));
static void destruct(void) __attribute__ ((destructor));
static void construct(void) {}
static void destruct(void) {}
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
