/** @file       lisp.c
 *  @brief      A minimal lisp interpreter and utility library, interface
 *              functions.
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com**/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

void lisp_throw(lisp_t * l, int ret)
{
	if (l && !l->errors_halt && l->recover_init)
		longjmp(l->recover, ret);
	else
		exit(ret);
}

int lisp_add_module_subroutines(lisp_t *l, lisp_module_subroutines_t *ms, size_t len)
{
	for(size_t i = 0; ms[i].name && (!len || i < len); i++)
		if(!lisp_add_subr(l, ms[i].name, ms[i].p, ms[i].validate, ms[i].docstring))
			return -1;
	return 0;
}

char *lisp_strdup(lisp_t *l, const char *s)
{
	assert(l && s);
	char *r = lstrdup(s);
	if(!r)
		LISP_HALT(l, "\"%s\"", "out of memory");
	return r;
}

lisp_cell_t *lisp_add_subr(lisp_t * l, const char *name, subr func, const char *fmt, const char *doc)
{
	assert(l && name && func);	/*fmt and doc are optional */
	return lisp_extend_top(l, lisp_intern(l, lisp_strdup(l, name)), mk_subr(l, func, fmt, doc));
}

lisp_cell_t *lisp_get_all_symbols(lisp_t * l)
{
	assert(l);
	return l->all_symbols;
}

lisp_cell_t *lisp_add_cell(lisp_t * l, const char *sym, lisp_cell_t * val)
{
	assert(l && sym && val);
	return lisp_extend_top(l, lisp_intern(l, lisp_strdup(l, sym)), val);
}

void lisp_destroy(lisp_t * l)
{
	if (!l)
		return;
	free(l->buf);
	l->gc_off = 0;
	if (l->gc_stack)
		lisp_gc_sweep_only(l), free(l->gc_stack);
	if (lisp_get_logging(l))
		io_close(lisp_get_logging(l));
	if (lisp_get_output(l))
		io_close(lisp_get_output(l));
	if (lisp_get_input(l))
		io_close(lisp_get_input(l));
	free(l->input);
	free(l->output);
	free(l->logging);
	free(l);
}

lisp_cell_t *lisp_read(lisp_t * l, io_t * i)
{
	assert(l && i);
	lisp_cell_t *ret;
	int restore_used, r;
	jmp_buf restore;
	if (l->recover_init) {
		memcpy(restore, l->recover, sizeof(jmp_buf));
		restore_used = 1;
	}
	if ((r = setjmp(l->recover))) {
		LISP_RECOVER_RESTORE(restore_used, l, restore);
		return r > 0 ? l->error : NULL;
	}
	l->recover_init = 1;
	ret = reader(l, i);
	LISP_RECOVER_RESTORE(restore_used, l, restore);
	return ret;
}

int lisp_print(lisp_t * l, lisp_cell_t * ob)
{
	assert(l && ob);
	int ret = printer(l, lisp_get_output(l), ob, 0);
	io_putc('\n', lisp_get_output(l));
	return ret;
}

lisp_cell_t *lisp_eval(lisp_t * l, lisp_cell_t * exp)
{
	assert(l && exp);
	lisp_cell_t *ret;
	int restore_used, r;
	jmp_buf restore;
	if (l->recover_init) {
		memcpy(restore, l->recover, sizeof(jmp_buf));
		restore_used = 1;
	}
	if ((r = setjmp(l->recover))) {
		LISP_RECOVER_RESTORE(restore_used, l, restore);
		return r > 0 ? l->error : NULL;
	}
	l->recover_init = 1;
	ret = eval(l, 0, exp, l->top_env);
	LISP_RECOVER_RESTORE(restore_used, l, restore);
	return ret;
}

/**@bug the entire string should be evaluated, not just the first expression */
lisp_cell_t *lisp_eval_string(lisp_t * l, const char *evalme)
{
	assert(l && evalme);
	io_t *in = NULL;
	lisp_cell_t *ret;
	volatile int restore_used = 0, r;
	jmp_buf restore;
	if (!(in = io_sin(evalme)))
		return NULL;
	if (l->recover_init) {
		memcpy(restore, l->recover, sizeof(jmp_buf));
		restore_used = 1;
	}
	if ((r = setjmp(l->recover))) {
		io_close(in);
		LISP_RECOVER_RESTORE(restore_used, l, restore);
		return r > 0 ? l->error : NULL;
	}
	l->recover_init = 1;
	ret = eval(l, 0, reader(l, in), l->top_env);
	io_close(in);
	LISP_RECOVER_RESTORE(restore_used, l, restore);
	return ret;
}

int lisp_log_error(lisp_t *l, char *fmt, ...) 
{
	int ret = 0;
	if(lisp_get_log_level(l) >= LISP_LOG_LEVEL_ERROR) {
		va_list ap;
		io_t *e = lisp_get_logging(l);
		va_start(ap, fmt);
		lisp_printf(l, e, 0, "(%rerror%t ");
		ret = lisp_vprintf(l, lisp_get_logging(l), 0, fmt, ap);
		lisp_printf(l, e, 0, ")%t\n");
		va_end(ap);
	}
	return ret;
}

int lisp_log_note(lisp_t *l, char *fmt, ...) 
{
	int ret = 0;
	if(lisp_get_log_level(l) >= LISP_LOG_LEVEL_NOTE) {
        	va_list ap;
		io_t *e = lisp_get_logging(l);
        	va_start(ap, fmt);
		lisp_printf(l, e, 0, "(%ynote%t ");
		ret = lisp_vprintf(l, e, 0, fmt, ap);
		lisp_printf(l, e, 0, ")%t\n");
		va_end(ap);
	}
	return ret;
}

int lisp_log_debug(lisp_t *l, char *fmt, ...) 
{
	int ret = 0;
	if(lisp_get_log_level(l) >= LISP_LOG_LEVEL_DEBUG) {
		va_list ap;
		io_t *e = lisp_get_logging(l);
		va_start(ap, fmt);
		lisp_printf(l, e, 0, "(%mdebug%t ");
		ret = lisp_vprintf(l, e, 0, fmt, ap);
		lisp_printf(l, e, 0, ")%t\n");
		va_end(ap);
	}
	return ret;
}

int lisp_set_input(lisp_t * l, io_t * in)
{
	assert(l);
	l->input->p[0].v = in;
	if (!in || !io_is_in(in))
		return -1;
	return 0;
}

int lisp_set_output(lisp_t * l, io_t * out)
{
	assert(l);
	l->output->p[0].v = out;
	if (!out || !io_is_out(out))
		return -1;
	return 0;
}

int lisp_set_logging(lisp_t * l, io_t * logging)
{
	assert(l);
	l->logging->p[0].v = logging;
	if (!logging || !io_is_out(logging))
		return -1;
	return 0;
}

void lisp_set_line_editor(lisp_t * l, lisp_editor_func ed)
{
	assert(l);
	l->editor = ed;
}

void lisp_set_signal(lisp_t * l, int sig)
{
	assert(l);
	l->sig = sig;
}

io_t *lisp_get_input(lisp_t * l)
{
	assert(l);
	return get_io(l->input);
}

io_t *lisp_get_output(lisp_t * l)
{
	assert(l);
	return get_io(l->output);
}

io_t *lisp_get_logging(lisp_t * l)
{
	assert(l);
	return get_io(l->logging);
}

void lisp_set_log_level(lisp_t *l, lisp_log_level level)
{
	assert(l && level < LISP_LOG_LEVEL_LAST_INVALID);
	l->log_level = level;
}

lisp_log_level lisp_get_log_level(lisp_t *l)
{
	assert(l);
	return l->log_level;
}

