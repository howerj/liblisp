/** @file       read.c
 *  @brief      An S-Expression parser for liblisp
 *  @author     Richard Howe (2015, 2016)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com 
 *  
 *  An S-Expression parser, it takes it's input from a generic input
 *  port that can be set up to read from a string or a file.
 *
 *  The hash table arguments are all not evaluated, but the primitive
 *  "hash-create" can be used to create a table where the arguments are
 *  evaluated.
 **/
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
    parse_hashes = 1, 		/*parse hashes? e.g. { a b c (1 2 3) } */
    parse_dotted = 1 /*parse dotted pairs? e.g. (a . b) */ ;

static int comment(io_t * i)
{			    /**@brief process a comment from I/O stream**/
	int c;
	while (((c = io_getc(i)) > 0) && (c != '\n')) ;
	return c;
}

/**@brief add a char to the token buffer*/
static void add_char(lisp_t * l, char ch)
{
	assert(l);
	char *tmp;
	if (l->buf_used > l->buf_allocated - 1) {
		l->buf_allocated = l->buf_used * 2;
		if (l->buf_allocated < l->buf_used)
			LISP_HALT(l, "%s", "overflow in allocator size variable");
		if (!(tmp = realloc(l->buf, l->buf_allocated)))
			LISP_HALT(l, "%s", "out of memory");
		l->buf = tmp;
	}
	l->buf[l->buf_used++] = ch;
}

/**@brief allocate a new token */
static char *new_token(lisp_t * l)
{
	assert(l);
	l->buf[l->buf_used++] = '\0';
	return lisp_strdup(l, l->buf);
}

/**@brief push back a single token */
static void unget_token(lisp_t * l, char *token)
{
	assert(l && token);
	l->token = token;
	l->ungettok = 1;
}

static char *lexer(lisp_t * l, io_t * i)
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
	if (strchr("(){}\'\"", ch))
		return new_token(l);
	for (;;) {
		if ((ch = io_getc(i)) == EOF)
			end = 1;
		if (ch == '#' || ch == ';') {
			comment(i);
			continue;
		}		/*ugly */
		if (strchr("(){}\'\"", ch) || isspace(ch)) {
			io_ungetc(ch, i);
			return new_token(l);
		}
		if (end)
			return new_token(l);
		add_char(l, ch);
	}
}

/**@brief handle parsing a string*/
static char *read_string(lisp_t * l, io_t * i)
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
 fail:				LISP_RECOVER(l, "'invalid-escape-literal \"%s\"", num);
			case EOF:
				return NULL;
			default:
				LISP_RECOVER(l, "'invalid-escape-char \"%c\"", ch);
			}
		}
		if (ch == '"')
			return new_token(l);
		add_char(l, ch);
	}
	return NULL;
}

static int keyval(lisp_t * l, io_t * i, hash_table_t *ht, char *key)
{
	lisp_cell_t *val;
	if(!(val = reader(l, i)))
		return -1;
	if(hash_insert(ht, key, cons(l, mk_str(l, key), val)) < 0)
		return -1;
	return 0;
}

static lisp_cell_t *read_hash(lisp_t * l, io_t * i)
{
	hash_table_t *ht;
	char *token = NULL; 
	if (!(ht = hash_create(SMALL_DEFAULT_LEN))) /**@bug leaks memory on error*/
		LISP_HALT(l, "%s", "out of memory");
	for(;;) {
		token = lexer(l, i);
		if(!token)
			goto fail;
		switch(token[0]) {
		case '}': 
			free(token);
			return mk_hash(l, ht);
		case '(': 
		case ')': 
		case '{': 
		case '\'':
		case '.': 
			goto fail;
		case '"': 
		{
			char *key;
			free(token);
			token = NULL;
			if(!(key = read_string(l, i)))
				goto fail;
			if(keyval(l, i, ht, key) < 0)
				goto fail;
			continue;
		}
		default:
			if(parse_ints && is_number(token)) {
				goto fail;
			} else if(parse_floats && is_fnumber(token)) {
				goto fail;
			}

			if(keyval(l, i, ht, new_token(l)) < 0)
				goto fail;
			free(token);
			continue;
		}
	}
fail:
	if(token)
		LISP_RECOVER(l, "%y'invalid-hash-key%t %r\"%s\"%t", token);
	hash_destroy(ht);
	free(token);
	return NULL;
}

static lisp_cell_t *read_list(lisp_t * l, io_t * i);
lisp_cell_t *reader(lisp_t * l, io_t * i)
{
	assert(l && i);
	char *token = lexer(l, i), *fltend = NULL;
	double flt;
	lisp_cell_t *ret;
	if (!token)
		return NULL;
	switch (token[0]) {
	case '(':
		free(token);
		return read_list(l, i);
	case ')':
		free(token);
		LISP_RECOVER(l, "%r\"unmatched %s\"%t", "')'");
	case '{':
		if(!parse_hashes)
			goto nohash;
		free(token);
		return read_hash(l, i);
	case '}':
		if(!parse_hashes)
			goto nohash;
		free(token);
		LISP_RECOVER(l, "%r\"unmatched %s\"%t", "'}'");
	case '"':
	{
		char *s;
		if (!parse_strings)
			goto nostring;
		free(token);
		if(!(s = read_string(l, i)))
			return NULL;
		return mk_str(l, s);
	}
	case '\'':
		free(token);
		return mk_list(l, gsym_quote(), reader(l, i), NULL);
	default:
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
 nostring:
 nohash:
		ret = lisp_intern(l, token);
		if (get_sym(ret) != token)
			free(token);
		return ret;
	}
	return gsym_nil();
}

/**@brief read in a list*/
static lisp_cell_t *read_list(lisp_t * l, io_t * i)
{
	assert(l && i);
	char *token = lexer(l, i), *stok;
	lisp_cell_t *tmp;
	if (!token) 
		NULL;
	switch (token[0]) {
	case ')':
		free(token);
		return gsym_nil();
	case '}':
		free(token);
		return gsym_nil();
	case '.':
		if (!parse_dotted)
			goto nodots;
		if (!(tmp = reader(l, i)))
			return NULL;
		if (!(stok = lexer(l, i)))
			return NULL;
		if (strcmp(stok, ")")) {
			free(stok);
			LISP_RECOVER(l, "%y'invalid-cons%t %r\"%s\"%t", "unexpected right parenthesis");
		}
		free(token);
		free(stok);
		return tmp;
	default:
		break;
	}
 nodots:
	unget_token(l, token);
	if (!(tmp = reader(l, i)))
		return NULL;	/* force evaluation order */
	return cons(l, tmp, read_list(l, i));
}

