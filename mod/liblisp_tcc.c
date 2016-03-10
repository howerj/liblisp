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

#define X(NAME, SUBR, VALIDATION, DOCSTRING) static lisp_cell_t * SUBR (lisp_t *l, lisp_cell_t *args);
SUBROUTINE_XLIST		/*function prototypes for all of the built-in subroutines */
#undef X
#define X(NAME, SUBR, VALIDATION, DOCSTRING) { NAME, VALIDATION, MK_DOCSTR(#SUBR, DOCSTRING), SUBR },
static lisp_module_subroutines_t primitives[] = {
	SUBROUTINE_XLIST	/*all of the subr functions */
	{NULL, NULL, NULL, NULL}	/*must be terminated with NULLs */
};

#undef X

static int ud_tcc = 0;

static void ud_tcc_free(lisp_cell_t * f)
{
	tcc_delete(get_user(f));
	free(f);
}

static int ud_tcc_print(io_t * o, unsigned depth, lisp_cell_t * f)
{
	return lisp_printf(NULL, o, depth, "%B<compiler-state:%d>%t", get_user(f));
}

static lisp_cell_t *subr_compile(lisp_t * l, lisp_cell_t * args)
{
	if (!lisp_check_length(args, 3)
	    || !is_usertype(car(args), ud_tcc)
	    || !is_asciiz(CADR(args)) || !is_str(CADDR(args)))
		LISP_RECOVER(l, "\"expected (compile-state string string\" '%S", args);
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

static lisp_cell_t *subr_link(lisp_t * l, lisp_cell_t * args)
{
	if (!lisp_check_length(args, 2)
	    || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
		LISP_RECOVER(l, "\"expected (compile-state string)\" '%S", args);
	return tcc_add_library(get_user(car(args)), get_str(CADR(args))) < 0 ? gsym_error() : gsym_nil();
}

static lisp_cell_t *subr_compile_file(lisp_t * l, lisp_cell_t * args)
{
	if (!lisp_check_length(args, 2)
	    || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
		LISP_RECOVER(l, "\"expected (compile-state string)\" '%S", args);
	if (tcc_add_file(get_user(car(args)), get_str(CADR(args))) < 0)
		return gsym_error();
	if (tcc_relocate(get_user(car(args)), TCC_RELOCATE_AUTO) < 0)
		return gsym_error();
	return gsym_tee();
}

static lisp_cell_t *subr_get_subr(lisp_t * l, lisp_cell_t * args)
{
	subr func;
	if (!lisp_check_length(args, 2)
	    || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
		LISP_RECOVER(l, "\"expected (compile-state string)\" '%S", args);
	if (!(func = tcc_get_symbol(get_user(car(args)), get_str(CADR(args)))))
		return gsym_error();
	else
		return mk_subr(l, func, NULL, NULL);
}

static lisp_cell_t *subr_add_include_path(lisp_t * l, lisp_cell_t * args)
{
	if (!lisp_check_length(args, 2) || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
		LISP_RECOVER(l, "\"expected (compile-state string)\" '%S", args);
	return tcc_add_include_path(get_user(car(args)), get_str(CADR(args))) < 0 ? gsym_error() : gsym_tee();
}

static lisp_cell_t *subr_add_sysinclude_path(lisp_t * l, lisp_cell_t * args)
{
	if (!lisp_check_length(args, 2) || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
		LISP_RECOVER(l, "\"expected (compile-state string)\" '%S", args);
	return tcc_add_sysinclude_path(get_user(car(args)), get_str(CADR(args))) < 0 ? gsym_error() : gsym_tee();
}

static lisp_cell_t *subr_set_lib_path(lisp_t * l, lisp_cell_t * args)
{
	if (!lisp_check_length(args, 2) || !is_usertype(car(args), ud_tcc) || !is_asciiz(CADR(args)))
		LISP_RECOVER(l, "\"expected (compile-state string)\" '%S", args);
	tcc_set_lib_path(get_user(car(args)), get_str(CADR(args)));
	return gsym_tee();
}

int lisp_module_initialize(lisp_t *l)
{
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

	if(lisp_add_module_subroutines(l, primitives, 0) < 0)
		goto fail;
	return 0;
 fail:	
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
