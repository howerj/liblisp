/** @file       io.c
 *  @brief      An input/output port wrapper
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com**/

#include "liblisp.h"
#include "private.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int io_is_in(io *i) { assert(i);
        return (i->type == FIN || i->type == SIN); 
}

int io_is_out(io *o) { assert(o); 
        return (o->type == FOUT || o->type == SOUT || o->type == NULLOUT); 
}

int io_is_file(io *f) { assert(f);
        return (f->type == FIN || f->type == FOUT);
}

int io_is_string(io *s) { assert(s);
        return (s->type == SIN || s->type == SOUT);
}

int io_is_null(io *n) { assert(n);
        return n->type == NULLOUT;
}

int io_getc(io *i) { assert(i);
        int r;
        if(i->ungetc)
                return i->ungetc = 0, i->c;
        if(i->type == FIN) { 
                if((r = fgetc(i->p.file)) == EOF)
                        i->eof = 1;
                return r;
        }
        if(i->type == SIN) 
                return i->p.str[i->position] ? i->p.str[i->position++] : EOF;
        FATAL("unknown or invalid IO type");
        return i->eof = 1, EOF;
}

char* io_get_string(io *x) { assert(x && io_is_string(x)); return x->p.str; }
FILE* io_get_file(io *x)   { assert(x && io_is_file(x));   return x->p.file; }

int io_ungetc(char c, io *i) { assert(i);
        if(i->ungetc) return i->eof = 1, EOF;
        i->c = c;
        i->ungetc = 1;
        return c;
}

int io_putc(char c, io *o) { assert(o);
        int r;
        char *p;
        size_t maxt;
        if(o->type == FOUT) {
                if((r = fputc(c, o->p.file)) == EOF)
                        o->eof = 1;
                return r;
        }
        if(o->type == SOUT) {
                if(o->position >= (o->max - 1)) { /*grow the "file"*/
                        maxt = (o->max+1) * 2;
                        if(maxt < o->position) /*overflow*/
                                return o->eof = 1, EOF;
                        o->max = maxt;       
                        if(!(p = realloc(o->p.str, maxt)))
                                return o->eof = 1, EOF;
                        memset(p + o->position, 0, maxt - o->position);
                        o->p.str = p;
                }
                o->p.str[o->position++] = c;
                return c;
        }
        if(o->type == NULLOUT)
                return c;
        FATAL("unknown or invalid IO type");
        return o->eof = 1, EOF;
}

int io_puts(const char *s, io *o) { assert(s && o);
        int r;
        char *p;
        size_t maxt;
        if(o->type == FOUT) {
                if((r = fputs(s, o->p.file)) == EOF)
                        o->eof = 1;
                return r;
        }
        if(o->type == SOUT) {
                size_t len = strlen(s), newpos;
                if(o->position + len >= (o->max - 1)) { /*grow the "file"*/
                        maxt = (o->position + len) * 2;
                        if(maxt < o->position) /*overflow*/
                                return o->eof = 1, EOF;
                        o->max = maxt;       
                        if(!(p = realloc(o->p.str, maxt)))
                                return o->eof = 1, EOF;
                        memset(p + o->position, 0, maxt - o->position);
                        o->p.str = p;
                }
                newpos = o->position + len;
                if(newpos >= o->max)
                        len = newpos - o->max;
                memmove(o->p.str + o->position, s, len);
                o->position = newpos;
                return len;
        }
        if(o->type == NULLOUT)
                return (int)strlen(s);
        FATAL("unknown or invalid IO type");
        return EOF;
}

char *io_getdelim(io *i, int delim) { assert(i);
        char *newbuf, *retbuf = NULL;
        size_t nchmax = 1, nchread = 0;
        int c;
        if(!(retbuf = calloc(1,1))) return NULL;
        while((c = io_getc(i)) != EOF) {
                if(nchread >= nchmax) { 
                        nchmax = nchread * 2;
                        if(nchread >= nchmax) /*overflow check*/
                                return free(retbuf), NULL;
                        if(!(newbuf = realloc(retbuf, nchmax + 1)))
                                return free(retbuf), NULL;
                        retbuf = newbuf;
                }
                if(c == delim) break;
                retbuf[nchread++] = c;
        }
        if(!nchread && c == EOF)
                return free(retbuf), NULL;
        if(retbuf)
                retbuf[nchread] = '\0';
        return retbuf;
}

char *io_getline(io *i) { assert(i); return io_getdelim(i, '\n'); }

int io_printd(intptr_t d, io * o) { assert(o);
        if(o->type == FOUT)
                return fprintf(o->p.file, "%"PRIiPTR, d);
        if(o->type == SOUT) {
                char dstr[64] = ""; 
                sprintf(dstr, "%"SCNiPTR, d);
                return io_puts(dstr, o);
        }
        return EOF;
}

int io_printflt(double f, io * o) { assert(o);
        if(o->type == FOUT)
                return fprintf(o->p.file, "%f", f);
        if(o->type == SOUT) {
                char dstr[512] = ""; /*floats can be very big!*/
                sprintf(dstr, "%f", f);
                return io_puts(dstr, o);
        }
        return EOF;
}

io *io_sin(const char *sin) {
        io *i;
        if(!sin || !(i = calloc(1, sizeof(*i)))) return NULL;
        if(!(i->p.str = lstrdup(sin))) return NULL;
        i->type = SIN;
        i->max = strlen(sin);
        return i;
}

io *io_fin(FILE *fin) { 
        io *i;
        if(!fin || !(i = calloc(1, sizeof(*i)))) return NULL;
        i->p.file = fin;
        i->type = FIN;
        return i;
}

io *io_sout(char *sout, size_t len) {
        io *o;
        if(!sout || !(o = calloc(1, sizeof(*o)))) return NULL;
        o->p.str = sout;
        o->type = SOUT;
        o->max = len;
        return o;
}

io *io_fout(FILE *fout) {
        io *o;
        if(!fout || !(o = calloc(1, sizeof(*o)))) return NULL;
        o->p.file = fout;
        o->type = FOUT;
        return o;
}

io *io_nout(void) {
        io *o;
        if(!(o = calloc(1, sizeof(*o)))) return NULL;
        o->type = NULLOUT;
        return o;
}

int io_close(io *c) {
        int ret = 0;
        if(!c) return -1;
        if(c->type == FIN || c->type == FOUT)
                if(c->p.file != stdin 
                && c->p.file != stdout 
                && c->p.file != stderr)
                        ret = fclose(c->p.file);
        if(c->type == SIN)
                free(c->p.str);
        free(c);
        return ret;
}

int io_eof(io *f) { assert(f);
        if(f->type == FIN || f->type == FOUT) f->eof = feof(f->p.file) ? 1 : 0;
        return f->eof;
}

int io_flush(io *f) { assert(f);
        if(f->type == FIN || f->type == FOUT) return fflush(f->p.file);
        return 0;
}

long io_tell(io *f) { assert(f);
        if(f->type == FIN || f->type == FOUT) return ftell(f->p.file);
        if(f->type == SIN || f->type == SOUT) return f->position;
        return -1;
}

int io_seek(io *f, long offset, int origin) { assert(f);
        if(f->type == FIN || f->type == FOUT) 
                return fseek(f->p.file, offset, origin);
        if(f->type == SIN || f->type == SOUT) {
                if(!f->max) return -1;
                switch(origin) {
                case SEEK_SET: f->position = offset;  break;
                case SEEK_CUR: f->position += offset; break;
                case SEEK_END: f->position = f->max - offset; break;
                default: return -1;
                }
                return f->position = MIN(f->position, f->max);
        }
        return -1;
}

int io_error(io *f) { assert(f);
        if(f->type == FIN || f->type == FOUT) 
                return ferror(f->p.file);
        return 0;
}

void io_color(io *out, int color_on)   { assert(out); out->color = color_on; }
void io_pretty(io *out, int pretty_on) { assert(out); out->pretty = pretty_on; }

