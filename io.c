/** @file       io.c
 *  @brief      An input/output port wrapper
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *  @todo       set error flags for strings, also refactor code
 **/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int io_is_in(io_t * i)
{
	assert(i);
	return (i->type == IO_FIN || i->type == IO_SIN);
}

int io_is_out(io_t * o)
{
	assert(o);
	return (o->type == IO_FOUT || o->type == IO_SOUT || o->type == IO_NULLOUT);
}

int io_is_file(io_t * f)
{
	assert(f);
	return (f->type == IO_FIN || f->type == IO_FOUT);
}

int io_is_string(io_t * s)
{
	assert(s);
	return (s->type == IO_SIN || s->type == IO_SOUT);
}

int io_is_null(io_t * n)
{
	assert(n);
	return n->type == IO_NULLOUT;
}

int io_getc(io_t * i)
{
	assert(i);
	int r;
	if (i->ungetc)
		return i->ungetc = 0, i->c;
	if (i->type == IO_FIN) {
		if ((r = fgetc(i->p.file)) == EOF)
			i->eof = 1;
		return r;
	}
	if (i->type == IO_SIN)
		return i->position < i->max ? i->p.str[i->position++] : EOF;
	FATAL("unknown or invalid IO type");
	return i->eof = 1, EOF;
}

char *io_get_string(io_t * x)
{
	assert(x && io_is_string(x));
	return x->p.str;
}

FILE *io_get_file(io_t * x)
{
	assert(x && io_is_file(x));
	return x->p.file;
}

int io_ungetc(char c, io_t * i)
{
	assert(i);
	if (i->ungetc)
		return i->eof = 1, EOF;
	i->c = c;
	i->ungetc = 1;
	return c;
}

int io_putc(char c, io_t * o)
{
	assert(o);
	int r;
	char *p;
	size_t maxt;
	if (o->type == IO_FOUT) {
		if ((r = fputc(c, o->p.file)) == EOF)
			o->eof = 1;
		return r;
	}
	if (o->type == IO_SOUT) {
		if (o->position >= (o->max - 1)) {	/*grow the "file" */
			maxt = (o->max + 1) * 2;
			if (maxt < o->position)	/*overflow */
				return o->eof = 1, EOF;
			o->max = maxt;
			if (!(p = realloc(o->p.str, maxt)))
				return o->eof = 1, EOF;
			memset(p + o->position, 0, maxt - o->position);
			o->p.str = p;
		}
		o->p.str[o->position++] = c;
		return c;
	}
	if (o->type == IO_NULLOUT)
		return c;
	FATAL("unknown or invalid IO type");
	return o->eof = 1, EOF;
}

int io_puts(const char *s, io_t * o)
{
	assert(s && o);
	int r;
	if (o->type == IO_FOUT) {
		if ((r = fputs(s, o->p.file)) == EOF)
			o->eof = 1;
		return r;
	}
	if (o->type == IO_SOUT) {
		/*this "grow" functionality should be moved into a function*/
		char *p;
		size_t len = strlen(s), newpos, maxt;
		if (o->position + len >= (o->max - 1)) {/*grow the "file" */
			maxt = (o->position + len) * 2;
			if (maxt < o->position)	/*overflow */
				return o->eof = 1, EOF;
			o->max = maxt;
			if (!(p = realloc(o->p.str, maxt)))
				return o->eof = 1, EOF;
			memset(p + o->position, 0, maxt - o->position);
			o->p.str = p;
		}
		newpos = o->position + len;
		if (newpos >= o->max)
			len = newpos - o->max;
		memmove(o->p.str + o->position, s, len);
		o->position = newpos;
		return len;
	}
	if (o->type == IO_NULLOUT)
		return (int)strlen(s);
	FATAL("unknown or invalid IO type");
	return EOF;
}

size_t io_read(void *ptr, size_t size, size_t nmemb, io_t *i)
{ /**@todo test me, this function is untested*/
	if(i->type == IO_FIN)
		return fread(ptr, size, nmemb, io_get_file(i));
	if(i->type == IO_SIN) {
		size_t len = size * nmemb; 
		size_t copy = MIN(i->position + len, i->max);
		assert(size && len / size == nmemb); /*overflow check*/
		memcpy(ptr, i->p.str + i->position, copy);
		i->position += copy;
		return copy;
	}
	FATAL("unknown or invalid IO type");
	return 0;
}

size_t io_write(void *ptr, size_t size, size_t nmemb, io_t *o)
{ /**@todo test me, this function is untested*/
	if(o->type == IO_SOUT) {
		char *p;
		size_t len = size * nmemb, newpos, maxt; 
		assert(size && len / size == nmemb); /*overflow check*/
		if (o->position + len >= (o->max - 1)) {/*grow the "file" */
			maxt = (o->position + len) * 2;
			if (maxt < o->position)	/*overflow */
				return o->eof = 1, EOF;
			o->max = maxt;
			if (!(p = realloc(o->p.str, maxt)))
				return o->eof = 1, EOF;
			memset(p + o->position, 0, maxt - o->position);
			o->p.str = p;
		}
		newpos = o->position + len;
		if (newpos >= o->max)
			len = newpos - o->max;
		memmove(o->p.str + o->position, ptr, len);
		o->position = newpos;
		return len;
	}
	if(o->type == IO_FOUT) {
		return fwrite(ptr, size, nmemb, io_get_file(o));
	}
	if(o->type == IO_NULLOUT)
		return nmemb;
	FATAL("unknown or invalid IO type");
	return 0;
}

char *io_getdelim(io_t * i, int delim)
{
	assert(i);
	char *newbuf, *retbuf = NULL;
	size_t nchmax = 1, nchread = 0;
	int c;
	if (!(retbuf = calloc(1, 1)))
		return NULL;
	while ((c = io_getc(i)) != EOF) {
		if (nchread >= nchmax) {
			nchmax = nchread * 2;
			if (nchread >= nchmax)	/*overflow check */
				return free(retbuf), NULL;
			if (!(newbuf = realloc(retbuf, nchmax + 1)))
				return free(retbuf), NULL;
			retbuf = newbuf;
		}
		if (c == delim)
			break;
		retbuf[nchread++] = c;
	}
	if (!nchread && c == EOF)
		return free(retbuf), NULL;
	if (retbuf)
		retbuf[nchread] = '\0';
	return retbuf;
}

char *io_getline(io_t * i)
{
	assert(i);
	return io_getdelim(i, '\n');
}

int io_printd(intptr_t d, io_t * o)
{
	assert(o);
	if (o->type == IO_FOUT)
		return fprintf(o->p.file, "%" PRIiPTR, d);
	if (o->type == IO_SOUT) {
		char dstr[64] = "";
		sprintf(dstr, "%" SCNiPTR, d);
		return io_puts(dstr, o);
	}
	return EOF;
}

int io_printflt(double f, io_t * o)
{
	assert(o);
	if (o->type == IO_FOUT)
		return fprintf(o->p.file, "%e", f);
	if (o->type == IO_SOUT) {
		/**@note if using %f the numbers can printed can be very large (~512 characters long) */
		char dstr[32] = "";
		sprintf(dstr, "%e", f);
		return io_puts(dstr, o);
	}
	return EOF;
}

io_t *io_sin(const char *sin, size_t len)
{
	io_t *i;
	if (!sin || !(i = calloc(1, sizeof(*i))))
		return NULL;
	if (!(i->p.str = calloc(len, 1)))
		return NULL;
	memcpy(i->p.str, sin, len);
	i->type = IO_SIN;
	i->max = len;
	return i;
}

io_t *io_fin(FILE * fin)
{
	io_t *i;
	if (!fin || !(i = calloc(1, sizeof(*i))))
		return NULL;
	i->p.file = fin;
	i->type = IO_FIN;
	return i;
}

io_t *io_sout(size_t len)
{
	io_t *o;
	char *sout;
	len = len == 0 ? 1 : len;
	if (!(sout = calloc(len, 1)) || !(o = calloc(1, sizeof(*o))))
		return NULL;
	o->p.str = sout;
	o->type = IO_SOUT;
	o->max = len;
	return o;
}

io_t *io_fout(FILE * fout)
{
	io_t *o;
	if (!fout || !(o = calloc(1, sizeof(*o))))
		return NULL;
	o->p.file = fout;
	o->type = IO_FOUT;
	return o;
}

io_t *io_nout(void)
{
	io_t *o;
	if (!(o = calloc(1, sizeof(*o))))
		return NULL;
	o->type = IO_NULLOUT;
	return o;
}

int io_close(io_t * c)
{
	int ret = 0;
	if (!c)
		return -1;
	if (c->type == IO_FIN || c->type == IO_FOUT)
		if (c->p.file != stdin && c->p.file != stdout && c->p.file != stderr)
			ret = fclose(c->p.file);
	if (c->type == IO_SIN)
		free(c->p.str);
	free(c);
	return ret;
}

int io_eof(io_t * f)
{
	assert(f);
	if (f->type == IO_FIN || f->type == IO_FOUT)
		f->eof = feof(f->p.file) ? 1 : 0;
	return f->eof;
}

int io_flush(io_t * f)
{
	assert(f);
	if (f->type == IO_FIN || f->type == IO_FOUT)
		return fflush(f->p.file);
	return 0;
}

long io_tell(io_t * f)
{
	assert(f);
	if (f->type == IO_FIN || f->type == IO_FOUT)
		return ftell(f->p.file);
	if (f->type == IO_SIN || f->type == IO_SOUT)
		return f->position;
	return -1;
}

int io_seek(io_t * f, long offset, int origin)
{
	assert(f);
	if (f->type == IO_FIN || f->type == IO_FOUT)
		return fseek(f->p.file, offset, origin);
	if (f->type == IO_SIN || f->type == IO_SOUT) {
		if (!f->max)
			return -1;
		switch (origin) {
		case SEEK_SET:
			f->position = offset;
			break;
		case SEEK_CUR:
			f->position += offset;
			break;
		case SEEK_END:
			f->position = f->max - offset;
			break;
		default:
			return -1;
		}
		return f->position = MIN(f->position, f->max);
	}
	return -1;
}

int io_error(io_t * f)
{
	assert(f);
	if (f->type == IO_FIN || f->type == IO_FOUT)
		return ferror(f->p.file);
	return 0;
}

void io_color(io_t * out, int color_on)
{
	assert(out);
	out->color = color_on;
}

void io_pretty(io_t * out, int pretty_on)
{
	assert(out);
	out->pretty = pretty_on;
}

