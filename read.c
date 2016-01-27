/** @file       read.c
 *  @brief      An S-Expression parser for liblisp
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com 
 *  
 *  An S-Expression parser, it takes it's input from a generic input
 *  port that can be set up to read from a string or a file.
 *
 *  @todo Error messages could be improved. Some kind of limited evaluation 
 *        could go here as well. Giving meaning to different forms of parenthesis 
 *        (such as "{}", "<>", and "[]") could improve readability or be used
 *        for syntax.
 *  @note there is no option to strip colors of parsed input. **/
#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

/* These are options that control what gets parsed */
static const int parse_strings = 1,	/*parse strings? e.g. "Hello" */
    parse_floats = 1,		/*parse floating point numbers? e.g. 1.3e4 */
    parse_ints = 1,		/*parse integers? e.g. 3 */
    parse_dotted = 1 /*parse dotted pairs? e.g. (a . b) */ ;

static int comment(io * i)
{			    /**@brief process a comment from I/O stream**/
	int c;
	while (((c = io_getc(i)) > 0) && (c != '\n')) ;
	return c;
}

/**@brief add a char to the token buffer*/
static void add_char(lisp * l, char ch)
{
	assert(l);
	char *tmp;
	if (l->buf_used > l->buf_allocated - 1) {
		l->buf_allocated = l->buf_used * 2;
		if (l->buf_allocated < l->buf_used)
			HALT(l, "%s", "overflow in allocator size variable");
		if (!(tmp = realloc(l->buf, l->buf_allocated)))
			HALT(l, "%s", "out of memory");
		l->buf = tmp;
	}
	l->buf[l->buf_used++] = ch;
}

/**@brief allocate a new token */
static char *new_token(lisp * l)
{
	assert(l);
	char *s;
	l->buf[l->buf_used++] = '\0';
	if (!(s = lstrdup(l->buf)))
		HALT(l, "%s", "out of memory");
	return s;
}

/**@brief push back a single token */
static void unget_token(lisp * l, char *token)
{
	assert(l && token);
	l->token = token;
	l->ungettok = 1;
}

static char *lexer(lisp * l, io * i)
{
	assert(l && i);
	int ch, end = 0;
	l->buf_used = 0;
	if (l->ungettok)
		return l->ungettok = 0, l->token;
	do {
		if ((ch = io_getc(i)) == EOF)
			return NULL;
		if (ch == '#' || ch == ';') {
			comment(i);
			continue;
		}		/*ugly */
	} while (isspace(ch) || ch == '#' || ch == ';');
	add_char(l, ch);
	if (strchr("()\'\"", ch))
		return new_token(l);
	for (;;) {
		if ((ch = io_getc(i)) == EOF)
			end = 1;
		if (ch == '#' || ch == ';') {
			comment(i);
			continue;
		}		/*ugly */
		if (strchr("()\'\"", ch) || isspace(ch)) {
			io_ungetc(ch, i);
			return new_token(l);
		}
		if (end)
			return new_token(l);
		add_char(l, ch);
	}
}

/**@brief handle parsing a string*/
static cell *read_string(lisp * l, io * i)
{
	assert(l && i);					   
	int ch;
	char num[4] = { 0, 0, 0, 0 };
	l->buf_used = 0;
	for (;;) {
		if ((ch = io_getc(i)) == EOF)
			return NULL;
		if (ch == '\\') {
			ch = io_getc(i);
			switch (ch) {
			case '\\':
				add_char(l, '\\');
				continue;
			case 'n':
				add_char(l, '\n');
				continue;
			case 't':
				add_char(l, '\t');
				continue;
			case 'r':
				add_char(l, '\r');
				continue;
			case '"':
				add_char(l, '"');
				continue;
			case '0':
			case '1':
			case '2':
			case '3':
				num[0] = ch;
				if ((ch = io_getc(i)) == EOF)
					return NULL;
				num[1] = ch;
				if ((ch = io_getc(i)) == EOF)
					return NULL;
				num[2] = ch;
				if (num[strspn(num, "01234567")])
					goto fail;
				ch = (char)strtol(num, NULL, 8);
				if (!ch)	/*cannot handle NUL in strings! */
					goto fail;
				add_char(l, ch);
				continue;
 fail:				RECOVER(l, "'invalid-escape-literal \"%s\"", num);
			case EOF:
				return NULL;
			default:
				RECOVER(l, "'invalid-escape-char \"%c\"", ch);
			}
		}
		if (ch == '"')
			return mk_str(l, new_token(l));
		add_char(l, ch);
	}
	return gsym_nil();
}

static cell *readlist(lisp * l, io * i);
cell *reader(lisp * l, io * i)
{
	assert(l && i);
	char *token = lexer(l, i), *fltend = NULL;
	double flt;
	cell *ret;
	if (!token)
		return NULL;
	switch (token[0]) {
	case ')':
		free(token);
		RECOVER(l, "%r\"unmatched %s\"%t", "')'");
	case '(':
		free(token);
		return readlist(l, i);
	case '"':
		if (!parse_strings)
			goto nostring;
		free(token);
		return read_string(l, i);
	case '\'':
		free(token);
		return mk_list(l, gsym_quote(), reader(l, i), NULL);
	default:
 nostring:
		if (parse_ints && is_number(token)) {
			ret = mk_int(l, strtol(token, NULL, 0));
			free(token);
			return ret;
		}
		if (parse_floats && is_fnumber(token)) {
			flt = strtod(token, &fltend);
			if (!fltend[0]) {
				free(token);
				return mk_float(l, flt);
			}
		}
		ret = intern(l, token);
		if (get_sym(ret) != token)
			free(token);
		return ret;
	}
	return gsym_nil();
}

/**@brief read in a list*/
static cell *readlist(lisp * l, io * i)
{
	assert(l && i);
	char *token = lexer(l, i), *stok;
	cell *tmp;
	if (!token)
		return NULL;
	switch (token[0]) {
	case ')':
		return free(token), gsym_nil();
	case '.':
		if (!parse_dotted)
			goto nodots;
		if (!(tmp = reader(l, i)))
			return NULL;
		if (!(stok = lexer(l, i)))
			return NULL;
		if (strcmp(stok, ")")) {
			free(stok);
			RECOVER(l, "%y'invalid-cons%t %r\"%s\"%t", "unexpected right parenthesis");
		}
		free(token);
		free(stok);
		return tmp;
	default:
		break;
	}
 nodots:unget_token(l, token);
	if (!(tmp = reader(l, i)))
		return NULL;	/* force evaluation order */
	return cons(l, tmp, readlist(l, i));
}
