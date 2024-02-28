/** @file       print.c
 *  @brief      print out S-expressions
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *  @warning    Colorization supported with ANSI escape codes only **/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>

char *lisp_serialize(lisp_t *l, lisp_cell_t *x) {
	assert(l && x);
	char *rs = NULL;
	io_t *s = io_sout(2);
	if (!s)
		goto fail;
	if (printer(l, s, x, 0) < 0)
		goto fail;
	rs = io_get_string(s);
	io_close(s); /*this does not free the string it contains*/
	return rs;
fail:
	if (s) {
		free(io_get_string(s));
		io_close(s);
	}
	return NULL;
}

static int print_escaped_string(lisp_t * l, io_t * o, unsigned depth, char *s) {
	assert(l && o && s);
	int ret = 0, m = 0;
	char c;
	if ((ret = lisp_printf(l, o, depth, "%r\"")) < 0)
		return -1;
	while ((c = *s++)) {
		ret += m;
		switch (c) {
		case '\\':
			if ((m = lisp_printf(l, o, depth, "%m\\\\%r")) < 0)
				return -1;
			continue;
		case '\n':
			if ((m = lisp_printf(l, o, depth, "%m\\n%r")) < 0)
				return -1;
			continue;
		case '\t':
			if ((m = lisp_printf(l, o, depth, "%m\\t%r")) < 0)
				return -1;
			continue;
		case '\r':
			if ((m = lisp_printf(l, o, depth, "%m\\r%r")) < 0)
				return -1;
			continue;
		case '"':
			if ((m = lisp_printf(l, o, depth, "%m\\\"%r")) < 0)
				return -1;
			continue;
		default:
			break;
		}
		if (!isprint(c)) {
			char num[5] = "\\";
			sprintf(num + 1, "%03o", ((unsigned)c) & 0xFF);
			assert(!num[4]);
			if ((m = lisp_printf(l, o, depth, "%m%s%r", num)) < 0)
				return -1;
			continue;
		}
		if ((m = io_putc(c, o)) < 0)
			return -1;
	}
	if ((m = io_putc('"', o)) < 0)
		return -1;
	return ret + m;
}

int lisp_printf(lisp_t *l, io_t *o, unsigned depth, char *fmt, ...) {
	assert(l && fmt && o);
	va_list ap;
	va_start(ap, fmt);
	const int ret = lisp_vprintf(l, o, depth, fmt, ap);
	va_end(ap);
	return ret;
}

static int print_hash(lisp_t *l, io_t *o, unsigned depth, hash_table_t *ht) {
	int ret = 0, m = 0;
	size_t i;
	hash_entry_t *cur;
	if ((ret = lisp_printf(l, o, depth, "{")) < 0)
		return -1;
	for (i = 0; i < ht->len; i++)
		if (ht->table[i])
		/**@warning messy hash stuff*/
		for (cur = ht->table[i]; cur; cur = cur->next) {
			int n = 0;
			io_putc(' ', o);
			if (is_cons(cur->val) && is_sym(car(cur->val)))
				m = lisp_printf(l, o, depth, "%S", car(cur->val));
			else
				m = print_escaped_string(l, o, depth, cur->key);

			if (is_cons(cur->val))
				n = lisp_printf(l, o, depth, "%t %S", cdr(cur->val));
			else
				n = lisp_printf(l, o, depth, "%t %S", cur->val);
			if (m < 0 || n < 0)
				return -1;
			ret += m + n;
		}
	if ((m = io_puts(" }", o)) < 0)
		return -1;
	return ret + m;
}

int lisp_vprintf(lisp_t *l, io_t *o, unsigned depth, char *fmt, va_list ap) {
	intptr_t d;
	unsigned dep;
	double flt;
	char c, f, *s;
	int ret = 0;
	hash_table_t *ht;
	lisp_cell_t *ob;
	while (*fmt) {
		if (ret == EOF) goto finish;
		if ('%' == (f = *fmt++)) {
			switch (f = *fmt++) {
			case '\0':
				goto finish;
			case '%':
				ret = io_putc('%', o);
				break;
			case '@':
				f = *fmt++;
				if (!f) goto finish;
				dep = depth;
				while (dep--)
					ret = io_putc(f, o);
				break;
			case 'c':
				c = va_arg(ap, int);
				ret = io_putc(c, o);
				break;
			case 's':
				s = va_arg(ap, char*);
				ret = io_puts(s, o);
				break;
			case 'd':
				d = va_arg(ap, intptr_t);
				ret = io_printd(d, o);
				break;
			case 'f':
				flt = va_arg(ap, double);
				ret = io_printflt(flt, o);
				break;
			case 'S':
				ob = va_arg(ap, lisp_cell_t *);
				ret =  printer(l, o, ob, depth);
				break;
			case 'H':
				ht = va_arg(ap, hash_table_t *);
				ret = print_hash(l, o, depth, ht);
				break;
			default:
				if (o->color) {
					char *color = "";
					switch (f) {
					case 't': color = "\x1b[0m";  break; /*reset*/
					case 'B': color = "\x1b[1m";  break; /*bold text*/
					case 'v': color = "\x1b[7m";  break; /*reverse video*/
					case 'k': color = "\x1b[30m"; break; /*black*/
					case 'r': color = "\x1b[31m"; break; /*red*/
					case 'g': color = "\x1b[32m"; break; /*green*/
					case 'y': color = "\x1b[33m"; break; /*yellow*/
					case 'b': color = "\x1b[34m"; break; /*blue*/
					case 'm': color = "\x1b[35m"; break; /*magenta*/
					case 'a': color = "\x1b[36m"; break; /*cyan*/
					case 'w': color = "\x1b[37m"; break; /*white*/
					default: /*return -1:*/ break;
					}
					ret = io_puts(color, o);
				}
				break;
			}
		} else {
			ret = io_putc(f, o);
		}
	}
finish:
	return ret;
}

int printer(lisp_t *l, io_t *o, lisp_cell_t *op, unsigned depth) {
	lisp_cell_t *tmp;
	if (!op)
		return EOF;
	if (l && depth > MAX_RECURSION_DEPTH) {
		lisp_log_error(l, "%r'print-depth-exceeded %d%t", (intptr_t) depth);
		return -1;
	}
	switch (op->type) {
	case INTEGER:
		lisp_printf(l, o, depth, "%m%d", get_int(op));
		break;
	case FLOAT:
		lisp_printf(l, o, depth, "%m%f", get_float(op));
		break;
	case CONS:
		if (depth && o->pretty)
			lisp_printf(l, o, depth, "\n%@ ");
		if (op->mark) {
			op->mark = 0;
			lisp_printf(l, o, depth, "%g<recurse:%d>%t", (intptr_t)op);
			return 0;
		}
		tmp = op;
		op->mark = 1;
		io_putc('(', o);
		for (;;) {
			printer(l, o, car(op), depth + 1);
			if (is_nil(cdr(op))) {
				io_putc(')', o);
				break;
			}
			op = cdr(op);
			if (op->mark) {
				lisp_printf(l, o, depth, "%g <recurse:%d>%t)", (intptr_t)op);
				break;
			}
			if (!is_cons(op)) {
				lisp_printf(l, o, depth, " . %S)", op);
				break;
			}
			io_putc(' ', o);
		}
		tmp->mark = 0;
		break;
	case SYMBOL:
		if (is_nil(op)) lisp_printf(l, o, depth, "%rnil");
		else           lisp_printf(l, o, depth, "%y%s", get_sym(op));
		break;
	case STRING:
		print_escaped_string(l, o, depth, get_str(op));
		break;
	case SUBR:
		lisp_printf(l, o, depth, "%B<subroutine:%d>", get_int(op));
		break;
	case PROC: case FPROC:
		lisp_printf(l, o, depth+1,
			is_proc(op) ? "(%ylambda%t %S %S " :
				      "(%yflambda%t %S %S ",
					get_func_docstring(op), get_proc_args(op));
		for (tmp = get_proc_code(op); !is_nil(tmp); tmp = cdr(tmp)) {
			printer(l, o, car(tmp), depth+1);
			if (!is_nil(cdr(tmp)))
				io_putc(' ', o);
		}
		io_putc(')', o);
		break;
	case HASH:
		lisp_printf(l, o, depth, "%H", get_hash(op));
		break;
	case IO:
		lisp_printf(l, o, depth, "%B<io:%s:%d>",
			op->close? "closed" :
				(is_in(op)? "in" : "out"), get_int(op));
		break;
	case USERDEF:
		if (l && l->ufuncs[get_user_type(op)].print)
			(l->ufuncs[get_user_type(op)].print)(o, depth, op);
		else lisp_printf(l, o, depth, "<user:%d:%d>",
			get_user_type(op), get_int(op)); break;
	case INVALID:
	default:
		FATAL("internal inconsistency");
	}
	return lisp_printf(l, o, depth, "%t") == EOF ? EOF : 0;
}

