/** @file       liblisp.c
 *  @brief      A minimal lisp interpreter and utility library
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *  @brief      A small, extensible lisp interpreter see
 *
 *  @todo Add more assertions to function, subr_close
 *  @todo Env lookup should be split into top level hash and cons
 *        list for efficiency.
 *  @note struct hack could be applied strings and other types
 *  @todo There is a problem with recursive hashes!
 *  @todo User defined type printing function should be used
 *  @note Generic error handling for primitives before they are called?
 *  @note Profiling showed that the main offender is the assoc function
 *        which is not surprising.
 *  @todo Rewrite reader
 *  @note Add options for turning off parsing of strings and floats 
 *  @note There has to be a better way of handling the returns from
 *        longjmp, there is some macro magic to be had here.
 *
 *  The following functionality would be nice to put in to the core
 *  interpreter; support for matrices and complex numbers (c99 _Complex),
 *  UTF-8 support, text processing (diff, tr, grep, sed, à la perl), map
 *  and looping constructs from scheme, doc-strings, optional parsing of
 *  strings and float, apply, gensym, proper constant and a prolog engine. 
 *  But this is more of a wish list than anything.
 *
 *  There are a lot of things that need tidying up and rethinking about,
 *  especially when it comes to the interface of the library and the naming
 *  of functions. More comments are also in order. 
 *
 *  If you do not like the formatting use the "indent" program from: 
 *  <https://www.gnu.org/software/indent/>.
**/

#include "liblisp.h"
#include <assert.h>
#include <ctype.h>
#include <inttypes.h>
#include <locale.h>
#include <setjmp.h>
#include <signal.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define DEFAULT_LEN        (256)   /*just an arbitrary smallish number*/
#define MAX_USER_TYPES     (256)   /*max number of user defined types*/
#define COLLECTION_POINT   (1<<20) /*run gc after this many allocs */
#define UNUSED(X)          ((void)(X))
#define MAX(X, Y)          ((X) > (Y) ? (X) : (Y))
#define MIN(X, Y)          ((X) > (Y) ? (Y) : (X))
#define fatal(X)           warn(NULL, (X), __FILE__, __LINE__)

typedef enum lisp_type { INVALID, SYMBOL, INTEGER, CONS, PROC, SUBR, 
STRING, IO, HASH, FPROC, FLOAT, USERDEF } lisp_type;

/**@brief The level of tracing to do when evaluating objects for debugging**/
typedef enum trace_level { TRACE_OFF, TRACE_MARKED, TRACE_ALL} trace_level;
/**@brief Typedef for controlling the garbage collector**/
typedef enum gc_control { GC_ON, GC_POSTPONE, GC_OFF } gc_control;

typedef union lisp_union { /**<ideally we would use void * for everything*/
        void *v;     /**< use this for integers and points to cells*/
        lfloat f;    /**< if lfloat is double it could be bigger than *v */ 
        subr prim; /**< function pointers are not guaranteed to fit into a void**/
} lisp_union;

struct cell {
        unsigned type: 4, userdef: 8, mark: 1, uncollectable: 1, trace: 1, len :32;
        lisp_union p[1]; /*struct hack: use "lisp_union p[]" and change mk for c99*/
};

typedef struct hashentry {/**< linked list of entries in a bin*/
        char *key; /**< ASCII nul delimited string*/
        void *val; /**< arbitrary value*/
        struct hashentry *next; /*next in list!*/
} hashentry;

struct hashtable {
        size_t len;               /**< number of 'bins' in the hash table*/
        struct hashentry **table; /**< table of linked lists*/
};

struct io {
        union { FILE *file; char *str; } p;
        size_t position, max; /**< current position and max buf len*/
        enum { IO_INVALID, FIN, FOUT, SIN, SOUT, NULLOUT } type;
        unsigned ungetc:1, color:1, pretty :1, eof :1;
        char c; /**< one character of push back*/
};

typedef struct gc_list { 
        cell *ref; /**< reference to cell for the garbage collector to act on*/
        struct gc_list *next; /**< next in list*/
} gc_list; /**< type used to form linked list of all allocations*/

typedef struct userdef_funcs { 
        ud_free   free;  /**< to free a user defined type*/
        ud_mark   mark;  /**< to mark a user defined type*/
        ud_equal  equal; /**< to compare two user defined types*/
        ud_print  print; /**< to print two user defined types*/
} userdef_funcs; /**< functions the interpreter uses for user defined types*/

struct lisp {
        jmp_buf recover;
        io *ifp /*input*/, *ofp /*output*/, *efp /*error output*/;
        cell *all_symbols, *top_env, **gc_stack;
        gc_list *gc_head;
        char *token, *buf;
        size_t buf_allocated, buf_used, gc_allocated, gc_used, 
               gc_collectp, max_depth/*max recursion depth*/;
        uint64_t random_state[2];
        int sig, trace; /*set by signal handlers or other threads*/
        unsigned ungettok:1, recover_init:1, dynamic:1;
        gc_control gc_state;
        userdef_funcs ufuncs[MAX_USER_TYPES];
        int userdef_used; /*number of user defined types allocated*/
        editor_func editor; /**< line editor to use, optional*/
        /*options for the REPL*/
        unsigned color_on :1, debug_on :1, prompt_on: 1, editor_on :1;
};

#define CELL_LIST\
        X(Nil,     "nil")       X(Tee,     "t")\
        X(Quote,   "quote")     X(If,      "if")\
        X(Lambda,  "lambda")    X(Flambda, "flambda")\
        X(Define,  "define")    X(Set,     "set!")\
        X(Begin,   "begin")     X(Cond,    "cond")\
        X(Error,   "error")     X(Env,     "environment")\
        X(LetS,    "let*")

#define X(CNAME, LNAME) static cell _ ## CNAME = { SYMBOL, 0, 0, 1, 0, 0, .p[0].v = LNAME};
CELL_LIST /*structs for special cells*/
#undef X

#define X(CNAME, NOT_USED) static cell* CNAME = & _ ## CNAME;
CELL_LIST /*pointers to structs for special cells*/
#undef X

#define X(CNAME, NOT_USED) { & _ ## CNAME },
static struct special_cell_list { cell *internal; } special_cells[] = {
        CELL_LIST /*list of all special cells for initializer*/
        { NULL }
};
#undef X

/************************ generic helper functions ****************************/

#define FAILPRINTER(LISP, RET, FMT, ...) do{\
        printerf((LISP)->efp, 0, "(error '%s " FMT " \"%s\" %d)\n",\
                       __func__, __VA_ARGS__, __FILE__, (intptr_t)__LINE__);\
        throwf(l, RET);\
        } while(0)

#define RECOVER(LISP, FMT, ...) FAILPRINTER(LISP,  1, FMT, __VA_ARGS__)
#define HALT(LISP, FMT, ...)   FAILPRINTER(LISP, -1, FMT, __VA_ARGS__)

static void throwf(lisp *l, int ret) { /**@brief throw an exception**/
        if(l && l->recover_init) longjmp(l->recover, ret);
        else exit(ret);
}

static void warn(lisp *l, char *msg, char *file, long line) {
        fprintf(stderr, "(error \"%s\" \"%s\" %ld)\n", msg, file, line);
        throwf(l, 1);
}

char *lstrdup(const char *s) { assert(s);
        char *str;
        if(!(str = malloc(strlen(s) + 1))) 
                fatal("out of memory"); /*XXX: exit() in library!*/
        strcpy(str, s);
        return str;
}

int match(char *pat, char *str) { /*original @ http://c-faq.com/lib/regex.html*/
        if(!str) return 0;
again:  switch(*pat) {
        case '\0': return !*str;
        case '*':  return match(pat+1, str) || (*str && match(pat, str+1));
        case '?':  if(!*str)        return 0; pat++; str++; goto again;
        default:   if(*pat != *str) return 0; pat++; str++; goto again;
        }
}

uint32_t djb2(const char *s, size_t len) { assert(s); 
        uint32_t h = 5381;   /*magic number this hash uses, it just is*/
        size_t i = 0;
        for (i = 0; i < len; s++, i++) h = ((h << 5) + h) + (*s);
        return h;
}

char *getadelim(FILE *in, char delim) { assert(in);
        io io_in;
        memset(&io_in, 0, sizeof(in));
        io_in.p.file = in;
        io_in.type = FIN;
        return io_getdelim(&io_in, delim);
}

char *getaline(FILE *in) { assert(in); return getadelim(in, '\n'); }

char *lstrcatend(char *dest, const char *src) { assert(dest && src);
        size_t sz = strlen(dest);
        strcpy(dest + sz, src);
        return dest+sz;
}

char *vstrcatsep(const char *separator, const char *first, ...) { 
        size_t len, seplen, num = 0;
        char *retbuf, *p;
        va_list argp1, argp2;

        if (!separator || !first) return NULL;
        len = strlen(first);
        seplen = strlen(separator);

        va_start(argp1, first);
        va_copy(argp2, argp1);
        while ((p = va_arg(argp1, char *))) 
                 num++, len += strlen(p);
        va_end(argp1);

        len += (seplen * num);
        if (!(retbuf = malloc(len+1))) return NULL;
        strcpy(retbuf, first);

        va_start(argp2, first);
        while ((p = va_arg(argp2, char *)))
                 strcat(retbuf,separator), strcat(retbuf, p);
        va_end(argp2);
        return retbuf;
}

uint8_t binlog(unsigned long long v) { /*binary logarithm*/
        uint8_t r = 0;
        while(v >>= 1) r++;
        return r;
}

uint64_t xorshift128plus(uint64_t s[2]) { /*PRNG*/
	uint64_t x = s[0];
	uint64_t const y = s[1];
	s[0] = y;
	x ^= x << 23; /*a*/
	x ^= x >> 17; /*b*/
	x ^= y ^ (y >> 26); /*c*/
	s[1] = x;
	return x + y;
}

int balance(const char *sexpr) { assert(sexpr);
        int bal = 0, c;
        while((c=*sexpr++))
                if     (c == '(') bal++;
                else if(c == ')') bal--;
        return bal;
}

/************************ small hash library **********************************/

static uint32_t hash_alg(hashtable *table, const char *s) {
        return djb2(s, strlen(s)) % (table->len ? table->len : 1);
}

static hashentry *hash_newpair(const char *key, void *val) 
{ /**@brief internal function to create a chained hash node**/
        hashentry *np;
        if (!(np = calloc(1, sizeof(*np)))) return NULL;
        np->key = lstrdup(key);
        np->val = val;
        if (!np->key || !np->val) return NULL;
        return np;
}

hashtable *hash_create(size_t len) {
        hashtable *nt;
        if (!len || !(nt = calloc(1, sizeof(*nt)))) return NULL;
        if (!(nt->table  = calloc(1, sizeof(*nt->table)*len)))
                return free(nt), NULL;
        nt->len = len;
        return nt;
}

void hash_destroy(hashtable *h) {
        size_t i;
        hashentry *cur, *prev;
        if (!h) return;
        for(i = 0; i < h->len; i++)
                if(h->table[i]){
                        prev = NULL;
                        for(cur = h->table[i]; cur; cur = cur->next){
                                free(prev);
                                free(cur->key);
                                prev = cur;
                        }
                        free(prev);
                }
        free(h->table);
        free(h);
}

int hash_insert(hashtable *ht, const char *key, void *val) { assert(ht && key && val);
        uint32_t hash = hash_alg(ht, key);
        hashentry *cur = ht->table[hash], *newt, *last = NULL;

        for(;cur && cur->key && strcmp(key, cur->key); cur = cur->next)
                last = cur;

        if (cur && cur->key && !strcmp(key, cur->key)) {
                cur->val = val;
        } else {
                if(!(newt = hash_newpair(key, val))) 
                        return -1;
                if (cur == ht->table[hash]) {
                        newt->next = cur;
                        ht->table[hash] = newt;
                } else if (!cur) {
                        last->next = newt;
                } else {
                        newt->next = cur;
                        last->next = newt;
                }
        }
        return 0;
}

void *hash_foreach(hashtable *h, hash_func func) {
        size_t i;
        hashentry *cur;
        void *ret;
        if (!h || !func) return NULL;
        for(i = 0; i < h->len; i++)
                if(h->table[i])
                        for(cur = h->table[i]; cur; cur = cur->next)
                                if((ret = (*func)(cur->key, cur->val)))
                                        return ret;
        return NULL;
}

static void *hprint(const char *key, void *val) { assert(key);
        return printf("(\"%s\" %p)\n", key, val), NULL;
}

void hash_print(hashtable *h) { assert(h); hash_foreach(h, hprint); }

void* hash_lookup(hashtable *h, const char *key) {
        uint32_t hash;
        hashentry *cur;
        assert(h && key);

        hash = hash_alg(h, key);
        cur = h->table[hash];
        while (cur && cur->next && strcmp(cur->key, key))
                cur = cur->next;
        if(!cur || !cur->key || strcmp(cur->key, key)) return NULL;
        else return cur->val;
}

/***************************** I/O handling ***********************************/

int io_isin(io *i) { assert(i);
        return (i->type == FIN || i->type == SIN); 
}

int io_isout(io *o) { assert(o); 
        return (o->type == FOUT || o->type == SOUT || o->type == NULLOUT); 
}

int io_isfile(io *f) { assert(f);
        return (f->type == FIN || f->type == FOUT);
}

int io_isstring(io *s) { assert(s);
        return (s->type == SIN || s->type == SOUT);
}

int io_isnull(io *n) { assert(n);
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
        fatal("unknown or invalid IO type");
        return i->eof = 1, EOF;
}

int io_ungetc(char c, io *i) { assert(i);
        if(i->ungetc) return i->eof = 1, EOF;
        i->c = c;
        i->ungetc = 1;
        return c;
}

int io_putc(char c, io *o) { assert(o);
        int r;
        if(o->type == FOUT) {
                if((r = fputc(c, o->p.file)) == EOF)
                        o->eof = 1;
                return r;
        }
        if(o->type == SOUT) {
                if(o->position >= o->max)
                        return o->eof = 1, EOF;
                o->p.str[o->position++] = c;
                return c;
        }
        if(o->type == NULLOUT)
                return c;
        fatal("unknown or invalid IO type");
        return o->eof = 1, EOF;
}

int io_puts(const char *s, io *o) { assert(s && o);
        int r;
        if(o->type == FOUT) {
                if((r = fputs(s, o->p.file)) == EOF)
                        o->eof = 1;
                return r;
        }
        if(o->type == SOUT) {
                size_t len, newpos;
                if(o->position >= o->max)
                        return o->eof = 1, EOF;
                len = strlen(s);
                newpos = o->position + len;
                if(newpos >= o->max)
                        len = newpos - o->max;
                memmove(o->p.str + o->position, s, len);
                o->position = newpos;
                return len;
        }
        if(o->type == NULLOUT)
                return (int)strlen(s);
        fatal("unknown or invalid IO type");
        return EOF;
}

char *io_getdelim(io *i, char delim) { assert(i);
        char *newbuf, *retbuf = NULL;
        size_t nchmax = 0, nchread = 0;
        int c;
        /*from <http://c-faq.com/malloc/reallocnull.html>*/
        while((c = io_getc(i)) != EOF) {
                if(nchread >= nchmax) {
                        nchmax += 20;
                        if(nchread >= nchmax) /*overflow check*/
                                return free(retbuf), NULL;
                        if(!(newbuf = realloc(retbuf, nchmax + 1)))
                                return free(retbuf), NULL;
                        retbuf = newbuf;
                }
                if(c == delim) break;
                retbuf[nchread++] = c;
        }

        if(retbuf) {
                retbuf[nchread] = '\0';
                newbuf = realloc(retbuf, nchread + 1);
                if(newbuf)
                        retbuf = newbuf;
        }
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

io *io_sin(char *sin) {
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

/***************************** lisp lists *************************************/

static void gc_free(lisp *l, cell *ob) { /**< free a lisp cell*/
        if(ob->uncollectable) return;
        switch(ob->type) {
        case INTEGER: case CONS: case FLOAT:
        case PROC:    case SUBR: case FPROC:      free(ob); break;
        case STRING:  free(strval(ob));           free(ob); break;
        case SYMBOL:  free(symval(ob));           free(ob); break;
        case IO:      io_close(ioval(ob));        free(ob); break; 
        case HASH:    hash_destroy(hashval(ob));  free(ob); break;
        case USERDEF: if(l->ufuncs[ob->userdef].free)
                              (l->ufuncs[ob->userdef].free)(ob);
                      else free(ob);
                      break;
        case INVALID:
        default: fatal("internal inconsitency"); break;
        }
}

static void gc(lisp *l) {
        gc_list *v, **p;
        if(l->gc_state != GC_ON) return;
        for(p = &l->gc_head; *p != NULL;) {
                v = *p;
                if(v->ref->mark) {
                        p = &v->next;
                        v->ref->mark = 0;
                } else {
                        *p = v->next;
                        gc_free(l, v->ref);
                        free(v);
                }
        }
}

static cell *gc_add(lisp *l, cell* op) { /**< add a cell to the working set*/
        cell **olist;
        if(l->gc_state == GC_OFF) return op;
        if(l->gc_used++ > l->gc_allocated - 1) {
                l->gc_allocated = l->gc_used * 2;
                if(l->gc_allocated < l->gc_used) 
                        HALT(l, "%s", "overflow in allocator size variable");
                olist = realloc(l->gc_stack, l->gc_allocated * 
                                sizeof(*l->gc_stack));
                if(!olist)
                        HALT(l, "%s", "out of memory");
                l->gc_stack = olist;
        }
        l->gc_stack[l->gc_used - 1] = op; /**<anything reachable in here is not freed*/
        return op;
}

static void gc_mark(lisp *l, cell* op) { /**<recursively mark reachable cells*/
        if(l->gc_state != GC_ON) return;
        if(!op || op->uncollectable || op->mark) return;
        op->mark = 1;
        switch(op->type) {
        case INTEGER: case SYMBOL: case SUBR: 
        case STRING:  case IO:     case FLOAT:  break;
        case FPROC: case PROC: 
                   gc_mark(l, procargs(op)); 
                   gc_mark(l, proccode(op));
                   gc_mark(l, procenv(op));
                   break;
        case CONS: gc_mark(l, car(op));
                   gc_mark(l, cdr(op));
                   break;
        case HASH: {
                        size_t i;
                        hashentry *cur;
                        hashtable *h = hashval(op);
                        for(i = 0; i < h->len; i++)
                                if(h->table[i])
                                        for(cur = h->table[i]; cur; cur = cur->next)
                                                gc_mark(l, cur->val);
                   }
                   break;
        case USERDEF: if(l->ufuncs[op->userdef].mark)
                             (l->ufuncs[op->userdef].mark)(op);
                      break;
        case INVALID:
        default:   fatal("internal inconsistency: unknown type");
        }
}

static void gc_collect(lisp *l) {
        size_t i;
        gc_mark(l, l->all_symbols);
        gc_mark(l, l->top_env);
        for(i = 0; i < l->gc_used; i++)
                gc_mark(l, l->gc_stack[i]);
        gc(l);
        l->gc_collectp = 0;
}

static cell *mk(lisp *l, lisp_type type, size_t count, ...) 
{ /**@brief make new lisp cells and perform garbage bookkeeping/collection*/
        cell *ret;
        gc_list *node; /**< new node in linked list of all allocations*/
        va_list ap;
        size_t i;

        if(l->gc_collectp++ > COLLECTION_POINT) /*Set to 1 for testing*/
                if(l->gc_state == GC_ON) /*only collect when gc is on*/
                        gc_collect(l);

        va_start(ap, count);
        if(!(ret = calloc(1, sizeof(cell) + (count - 1) * sizeof(lisp_union))))
                HALT(l, "%s", "out of memory");
        if(!(node = calloc(1, sizeof(gc_list))))
                HALT(l, "%s", "out of memory");
        ret->type = type;
        for(i = 0; i < count; i++) 
                if     (FLOAT == type)  ret->p[i].f =    va_arg(ap, double);
                else if(SUBR == type) ret->p[i].prim = va_arg(ap, subr);
                else                    ret->p[i].v =    va_arg(ap, void*);
        va_end(ap);
        node->ref = ret;
        node->next = l->gc_head;
        l->gc_head = node;
        gc_add(l, ret);
        return ret;
}

cell* cons(lisp *l, cell *x, cell *y) {
        cell *z = mk(l, CONS, 2, x, y);
        if(!x || !y || !z) return NULL;
        if(y->type == CONS) z->len = y->len + 1;
        if(isnil(y)) z->len++;
        return z;
}

intptr_t intval(cell *x)     { return !x ? 0 : (intptr_t)(x->p[0].v); }
int  isnil(cell *x)          { return x == Nil; }
int  isint(cell *x)          { return !x ? 0 : x->type == INTEGER; }
int  isfloat(cell *x)        { return !x ? 0 : x->type == FLOAT; }
int  isio(cell *x)           { return !x ? 0 : x->type == IO; }
int  iscons(cell *x)         { return !x ? 0 : x->type == CONS; }
int  isproc(cell *x)         { return !x ? 0 : x->type == PROC; }
int  isfproc(cell *x)        { return !x ? 0 : x->type == FPROC; }
int  isstr(cell *x)          { return !x ? 0 : x->type == STRING; }
int  issym(cell *x)          { return !x ? 0 : x->type == SYMBOL; }
int  issubr(cell *x)       { return !x ? 0 : x->type == SUBR; }
int  ishash(cell *x)         { return !x ? 0 : x->type == HASH; }
int  isuserdef(cell *x)      { return !x ? 0 : x->type == USERDEF; }
int  isusertype(cell *x, int type) {
        if(!x) return 0;
        return x->type == USERDEF && x->userdef == type;
}
int  isasciiz(cell *x)       { return isstr(x) || issym(x); }
int  isarith(cell *x)        { return isint(x) || isfloat(x); }
cell *mkint(lisp *l, intptr_t d) { return mk(l, INTEGER, 1, (cell*) d); }
cell *mkio(lisp *l, io *x)   { return mk(l, IO, 1, (cell*)x); }
cell *mksubr(lisp *l, subr p) { return mk(l, SUBR, 1, p); }
cell *mkproc(lisp *l, cell *x, cell *y, cell *z)  { return mk(l,PROC,3,x,y,z); }
cell *mkfproc(lisp *l, cell *x, cell *y, cell *z) { return mk(l,FPROC,3,x,y,z); }
cell *mkfloat(lisp *l, lfloat f) { return mk(l, FLOAT, 1, f); }
cell *mkuser(lisp *l, void *x, int type) {
        cell *ret = mk(l, USERDEF, 1, x);
        ret->userdef = type;
        return ret;
}

static cell *mkasciiz(lisp *l, char *s, lisp_type type) {  
        assert(type == STRING || type == SYMBOL);
        cell *x = mk(l, type, 1, (cell *)s); 
        if(!x) return NULL;
        x->len = strlen(s);
        return x;
}

cell *mkstr(lisp *l, char *s){ return mkasciiz(l, s, STRING); }
cell *mksym(lisp *l, char *s){ return mkasciiz(l, s, SYMBOL); }
cell *mkhash(lisp *l, hashtable *h) { return mk(l, HASH, 1, (cell *)h); }
subr subrval(cell *x)    { return !x ? NULL : x->p[0].prim; }
cell *procargs(cell *x)       { return !x ? NULL : x->p[0].v; }
cell *proccode(cell *x)       { return !x ? NULL : x->p[1].v; }
cell *procenv(cell *x)        { return !x ? NULL : x->p[2].v; }
cell *car(cell *x)            { return !x ? NULL : x->p[0].v; }
cell *cdr(cell *x)            { return !x ? NULL : x->p[1].v; }
io*  ioval(cell *x)          { return !x ? NULL : (io*)(x->p[0].v); }
void setcar(cell *x, cell *y) { if(!x||!y) return;  x->p[0].v = y; }
void setcdr(cell *x, cell *y) { if(!x||!y) return;  x->p[1].v = y; }
char *symval(cell *x)        { return !x ? NULL : (char *)(x->p[0].v); }
char *strval(cell *x)        { return !x ? NULL : (char *)(x->p[0].v); }
void *userval(cell *x)       { return !x ? NULL : (void *)(x->p[0].v); }
const cell *mkerror(void)    { return Error; }
const cell *mknil(void)      { return Nil; }
const cell *mktee(void)      { return Tee; }
hashtable *hashval(cell *x){ return !x ? NULL : (hashtable *)(x->p[0].v); }
int  cklen(cell *x, size_t expect)  { return !x ? 0 : (x->len) == expect; }
lfloat floatval(cell *x)     { return !x ? 0.f : x->p[0].f; }

int newuserdef(lisp *l, ud_free f, ud_mark m, ud_equal e, ud_print p) {
        if(l->userdef_used >= MAX_USER_TYPES) return -1;
        l->ufuncs[l->userdef_used].free  = f;
        l->ufuncs[l->userdef_used].mark  = m;
        l->ufuncs[l->userdef_used].equal = e;
        l->ufuncs[l->userdef_used].print = p;
        return l->userdef_used++;
}

int isin(cell *x) {
        if(!x || !isio(x) 
              || (ioval(x)->type != FIN && ioval(x)->type != SIN)) return 0;
        return 1;
}

int isout(cell *x) { 
        if(!x || !isio(x) 
              ||    (ioval(x)->type != SOUT 
                 &&  ioval(x)->type != FOUT 
                 &&  ioval(x)->type != NULLOUT)) return 0;
        return 1;
}

cell *extend(lisp *l, cell *env, cell *sym, cell *val) { 
        return cons(l, cons(l, sym, val), env); 
}

cell *intern(lisp *l, char *name) {
        cell *op = hash_lookup(hashval(l->all_symbols), name);
        if(op) return op;
        op = mksym(l, name);
        hash_insert(hashval(l->all_symbols), name, op);
        return op;
}

/***************************** environment ************************************/

static cell *multiple_extend(lisp *l, cell *env, cell *syms, cell *vals) {
        if(!env || !syms || !vals) return NULL;
        for(;!isnil(syms); syms = cdr(syms), vals = cdr(vals))
                env = extend(l, env, car(syms), car(vals));
        return env;
}

static cell *extend_top(lisp *l, cell *sym, cell *val) {
        if(!sym || !val) return NULL;
        setcdr(l->top_env, cons(l, cons(l, sym, val), cdr(l->top_env)));
        return val;
}

static cell *assoc(cell *key, cell *alist) {
        /** @note improving the speed of this function, perhaps by
         *  adding support for hashes, would greatly increase the
         *  speed of the interpreter... **/
        /*can use "if(car(car(alist)) == key)" for symbol only comparison*/
        if(!key || !alist) return NULL;
        for(;!isnil(alist); alist = cdr(alist))
                if(intval(car(car(alist))) == intval(key)) 
                        return car(alist);
        return Nil;
}

/******************************** parsing *************************************/

int isnumber(const char *buf) { assert(buf);
        char conv[] = "0123456789abcdefABCDEF";
        if(!buf[0]) return 0;
        if(buf[0] == '-' || buf[0] == '+') buf++;
        if(!buf[0]) return 0;
        if(buf[0] == '0') { /*shorten the conv table depending on numbers base*/
                if(buf[1] == 'x' || buf[1] == 'X') conv[22] = '\0', buf+=2;
                else conv[8] = '\0';
        } else { conv[10] = '\0';}
        if(!buf[0]) return 0;
        return buf[strspn(buf, conv)] == '\0';
}

int isfnumber(const char *buf) { 
        size_t i;
        char conv[] = "0123456789";
        if(!buf[0]) return 0;
        if(buf[0] == '-' || buf[0] == '+') buf++;
        if(!buf[0]) return 0;
        i = strspn(buf, conv);
        if(buf[i] == '\0') return 1;
        if(buf[i] == 'e') goto expon; /*got check for valid exponentiation*/
        if(buf[i] != '.') return 0;
        buf = buf + i + 1;
        i = strspn(buf, conv);
        if(buf[i] == '\0') return 1;
        if(buf[i] != 'e' && buf[i] != 'E') return 0;
expon:  buf = buf + i + 1;
        if(buf[0] == '-' || buf[0] == '+') buf++;
        if(!buf[0]) return 0;
        i = strspn(buf, conv);
        if(buf[i] == '\0') return 1;
        return 0;
}

static int comment(io *i) { /**@brief process a comment from I/O stream**/
        int c; 
        while(((c = io_getc(i)) > 0) && (c != '\n')); 
        return c; 
} 

static void add_char(lisp *l, char ch)  {
        char *tmp; 
        if(l->buf_used > l->buf_allocated - 1) {
                l->buf_allocated = l->buf_used * 2;
                if(l->buf_allocated < l->buf_used)
                        HALT(l, "%s", "overflow in allocator size variable");
                if(!(tmp = realloc(l->buf, l->buf_allocated)))
                        HALT(l, "%s", "out of memory");
                l->buf = tmp;
        }
        l->buf[l->buf_used++] = ch; 
}

static char *terminate_string(lisp *l) {
        l->buf[l->buf_used++] = '\0'; 
        return lstrdup(l->buf);
}

static void ungettok(lisp *l, char *token) { 
        l->token = token; 
        l->ungettok = 1; 
}

static char *gettoken(lisp *l, io *i) { 
        int ch, end = 0;
        l->buf_used = 0;
        if(l->ungettok)
                return l->ungettok = 0, l->token; 
        do {
                if((ch = io_getc(i)) == EOF) 
                        return NULL;
                if(ch == '#') { comment(i); continue; } /*ugly*/
        } while(isspace(ch) || ch == '#');
        add_char(l, ch);
        if(strchr("()\'\"", ch))
                return terminate_string(l);
        for(;;) {
                if((ch = io_getc(i)) == EOF)
                        end = 1;
                if(ch == '#') { comment(i); continue; } /*ugly*/
                if(strchr("()\'\"", ch) || isspace(ch)) {
                        io_ungetc(ch, i);
                        return terminate_string(l);
                }
                if(end) return terminate_string(l);
                add_char(l, ch);
        }
}

static cell *readstring(lisp *l, io* i) {
        int ch;
        l->buf_used = 0;
        for(;;) {
                if((ch = io_getc(i)) == EOF)
                        return NULL;
                if(ch == '\\') {
                        ch = io_getc(i);
                        switch(ch) {
                        case '\\': add_char(l, '\\'); continue;
                        case 'n':  add_char(l, '\n'); continue;
                        case 't':  add_char(l, '\t'); continue;
                        case 'r':  add_char(l, '\r'); continue;
                        case '"':  add_char(l, '"');  continue;
                        case EOF:  return NULL;
                        default:  RECOVER(l, "'invalid-escape-char \"%c\"", ch);
                        }
                }
                if(ch == '"')
                        return mkstr(l, terminate_string(l));
                add_char(l, ch);
        }
        return Nil;
}

static cell *readlist(lisp *l, io *i);
static cell *reader(lisp *l, io *i) { /*read in s-expr*/
        char *token = gettoken(l, i), *fltend = NULL;
        double flt; 
        cell *ret;
        if(!token) return NULL;
        switch(token[0]) {
        case ')':  free(token); 
                   RECOVER(l, "\"unmatched %s\"", "')");
        case '(':  free(token); return readlist(l, i);
        case '"':  free(token); return readstring(l, i);
        case '\'': free(token); return cons(l, Quote, cons(l,reader(l,i),Nil));
        default:   if(isnumber(token)) {
                           ret = mkint(l, strtol(token, NULL, 0));
                           free(token);
                           return ret;
                   }
                   if(isfnumber(token)) {
                        flt = strtod(token, &fltend);
                        if(!fltend[0]) {
                                free(token);
                                return mkfloat(l, flt);
                        }
                   }
                   ret = intern(l, token);
                   if(symval(ret) != token) free(token);
                   return ret;
        }
        return Nil;
}

static cell *readlist(lisp *l, io *i) {
        char *token = gettoken(l, i), *stok;
        cell *tmp;
        if(!token) return NULL;
        switch(token[0]){
        case ')': return free(token), Nil;
        case '.': if(!(tmp = reader(l, i))) return NULL;
                  if(!(stok = gettoken(l, i))) return NULL;
                  if(strcmp(stok, ")")) {
                          free(stok);
                          RECOVER(l, "'invalid-cons \"%s\"", 
                                        "unexpected right parenthesis");
                  }
                  free(token);
                  free(stok);
                  return tmp;
        default:  break;
        }
        ungettok(l, token);
        if(!(tmp = reader(l, i))) return NULL; /* force evaluation order */
        return cons(l, tmp, readlist(l, i));
}

static int printer(io *o, cell *op, unsigned depth);
int printerf(io *o, unsigned depth, char *fmt, ...) {
        va_list ap;
        intptr_t d;
        unsigned dep;
        double flt;
        char c, f, *s;
        int ret = 0;
        hashtable *ht;
        cell *ob;
        va_start(ap, fmt);
        while(*fmt) {
                if(ret == EOF) goto finish;
                if('%' == (f = *fmt++)) {
                        switch (f = *fmt++) {
                        case '\0': goto finish;
                        case '%':  ret = io_putc('%', o); break;
                        case '*':  f = *fmt++;
                                   if(!f) goto finish;
                                   dep = depth;
                                   while(dep--)
                                           ret = io_putc(f, o); 
                                   break;
                        case 'c':  c = va_arg(ap, int);
                                   ret = io_putc(c, o);
                                   break;
                        case 's':  s = va_arg(ap, char*);
                                   ret = io_puts(s, o);
                                   break;
                        case 'd':  d = va_arg(ap, intptr_t);
                                   ret = io_printd(d, o); 
                                   break;
                        case 'f':  flt = va_arg(ap, double);
                                   ret = io_printflt(flt, o);
                                   break;
                        case 'S':  ob = va_arg(ap, cell*);
                                   ret =  printer(o, ob, depth);
                                   break;
                        case 'H':  ht = va_arg(ap, hashtable*);
                        {
                          size_t i;
                          hashentry *cur;
                          printerf(o, depth, "(%yhash-create%t");
                          for(i = 0; i < ht->len; i++)
                            if(ht->table[i])
                              for(cur = ht->table[i]; cur; cur = cur->next)
                                printerf(o, depth + 1, " '%s '%S", 
                                                        cur->key, cur->val);
                          ret = io_putc(')', o);
                        }
                        break;
                        default:   if(o->color) {
                                        char *color = "";
                                        switch(f) {
                     /*reset*/          case 't': color = "\x1b[0m";  break;
                     /*bold text*/      case 'B': color = "\x1b[1m";  break;
                     /*reverse video*/  case 'v': color = "\x1b[7m";  break;
                     /*black*/          case 'k': color = "\x1b[30m"; break;
                     /*red*/            case 'r': color = "\x1b[31m"; break;
                     /*green*/          case 'g': color = "\x1b[32m"; break;
                     /*yellow*/         case 'y': color = "\x1b[33m"; break;
                     /*blue*/           case 'b': color = "\x1b[34m"; break;
                     /*magenta*/        case 'm': color = "\x1b[35m"; break;
                     /*cyan*/           case 'a': color = "\x1b[36m"; break;
                     /*white*/          case 'w': color = "\x1b[37m"; break;
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
finish: va_end(ap);
        return ret;
}

static int printer(io *o, cell *op, unsigned depth) { /*write out s-expr*/
        if(!op) return EOF;
        switch(op->type) {
        case INTEGER: printerf(o, depth, "%m%d", intval(op));   break; 
        case FLOAT:   printerf(o, depth, "%m%f", floatval(op)); break; 
        case CONS:    if(depth && o->pretty) io_putc('\n', o);
                      if(o->pretty) printerf(o, depth, "%* ");
                      io_putc('(', o);
                      for(;;) {
                              printer(o, car(op), depth + 1);
                              if(isnil(cdr(op))) {
                                      io_putc(')', o);
                                      break;
                              }
                              op = cdr(op);
                              if(op->type != CONS) {
                                      printerf(o, depth, " . %S)", op);
                                      break;
                              }
                              io_putc(' ', o);
                      }
                      break;
        case SYMBOL:  if(isnil(op)) printerf(o, depth, "%r()");
                      else          printerf(o, depth, "%y%s", symval(op));
                      break;
        case STRING:{ char c, *s = strval(op);
                      printerf(o, depth, "%r\"");
                      while((c = *s++)) {
                        switch(c) {
                        case '\\': printerf(o, depth, "%m\\\\%r"); continue;
                        case '\n': printerf(o, depth, "%m\\n%r");  continue;
                        case '\t': printerf(o, depth, "%m\\t%r");  continue;
                        case '\r': printerf(o, depth, "%m\\r%r");  continue;
                        case '"':  printerf(o, depth, "%m\\\"%r"); continue;
                        default: break;
                        }
                        io_putc(c, o);
                      }
                      io_putc('"', o);
                      } break;
        case SUBR:    printerf(o, depth, "%B<SUBR:%d>", intval(op));  break;
        case PROC:    printerf(o, depth+1, "(%ylambda%t %S %S)", 
                                      procargs(op), car(proccode(op)));
                      break;
        case FPROC:   printerf(o, depth+1, "(%yflambda%t %S %S)", 
                                      procargs(op), car(proccode(op)));
                      break;
        case HASH:    printerf(o, depth, "%H",            hashval(op)); break;
        case IO:      printerf(o, depth, "%B<IO:%s:%d>",  
                                      isin(op)? "IN":"OUT", intval(op)); break;
        case USERDEF: /*XXX: replace with print functions for userdef from lisp *l */
                      printerf(o, depth, "<USER:%d:%d>",
                                (intptr_t)op->userdef,intval(op)); break;
        case INVALID: 
        default:      fatal("internal inconsistency");
        }
        return printerf(o, depth, "%t") == EOF ? EOF : 0;
}

/******************************** evaluator ***********************************/

static cell *evlis(lisp *l, unsigned depth, cell *exps, cell *env);
static cell *eval(lisp *l, unsigned depth, cell *exp, cell *env) { 
        /**@brief internal evaluator**/
        size_t gc_point = l->gc_used;
        cell *tmp, *first, *proc, *vals;
        if(!exp || !env) return NULL;
        if(depth > l->max_depth)
                RECOVER(l, "'recursion-depth-reached %d", depth);
        gc_add(l, exp);
        gc_add(l, env);

        tail:
        if(l->trace != TRACE_OFF) { /*print out expressions for debugging*/
                if(l->trace == TRACE_ALL || (l->trace == TRACE_MARKED && exp->trace))
                        printerf(l->efp, 1, "(%ytrace%t %S)\n", exp); 
                else if(l->trace != TRACE_MARKED)
                        HALT(l, "\"invalid trace level\" %d", l->trace);
        }
        if(exp == Nil) return Nil;
        if(l->sig) {
                l->sig = 0;
                throwf(l, 1);
        }

        switch(exp->type) {
        case INTEGER: case SUBR: case PROC:  case STRING: case FLOAT:
        case IO:      case HASH: case FPROC: case USERDEF:
                return exp; /*self evaluating types*/
        case SYMBOL:   
                if(isnil(tmp = assoc(exp, env)))
                        RECOVER(l, "\"unbound symbol\" '%s", symval(exp));
                return cdr(tmp);
        case CONS:
                first = car(exp);
                exp   = cdr(exp);
                if(first == If) {
                        if(!cklen(exp, 3))
                                RECOVER(l, "'if \"argc != 3 in %S\"", exp);
                        exp = !isnil(eval(l, depth+1, car(exp), env)) ?
                                car(cdr(exp)) :
                                car(cdr(cdr(exp)));
                        goto tail;
                }
                if(first == Lambda) {
                        if(!cklen(exp, 2))
                                RECOVER(l, "'lambda \"argc != 2 in %S\"", exp);
                        l->gc_used = gc_point;
                        tmp = mkproc(l, car(exp), cdr(exp), env);
                        return gc_add(l, tmp);
                }
                if(first == Flambda) {
                        if(!cklen(exp, 2)) 
                                RECOVER(l, "'flambda \"argc != 2 in %S\"", exp);
                        if(!cklen(car(exp), 1)) 
                                RECOVER(l, "'flambda \"only one argument allowed %S\"", exp);
                        l->gc_used = gc_point;
                        return gc_add(l, mkfproc(l, car(exp), cdr(exp), env));
                }
                if(first == Cond) {
                        if(cklen(exp, 0)) return Nil;
                        for(tmp=Nil; isnil(tmp) && !isnil(exp); exp=cdr(exp)) {
                                if(!iscons(car(exp))) return Nil;
                                tmp = eval(l, depth+1, car(car(exp)), env);
                                if(!isnil(tmp)) {
                                        exp = car(cdr(car(exp)));
                                        goto tail;
                                }
                        }
                        return Nil;
                }
                if(first == Env)   return env;
                if(first == Quote) return car(exp);
                if(first == Error) {
                        if(cklen(exp, 1) && isint(car(exp)))
                                throwf(l, intval(car(exp)));
                        if(cklen(exp, 0))
                                throwf(l, -1);
                        RECOVER(l, "'throw \"expected () or (int)\" '%S", exp); 
                }
                if(first == Define) {
                        if(!cklen(exp, 2))
                                RECOVER(l, "'define \"argc != 2 in %S\"", exp);
                        l->gc_used = gc_point;
                        return gc_add(l, extend_top(l, car(exp),
                              eval(l, depth+1, car(cdr(exp)), env)));
                }
                if(first == Set) {
                        cell *pair, *newval;
                        if(!cklen(exp, 2))
                                RECOVER(l, "'set! \"argc != 2 in %S\"", exp);
                        if(isnil(pair = assoc(car(exp), env)))
                                RECOVER(l, "'set! \"undefined variable\" '%S", exp);
                        newval = eval(l, depth+1, car(cdr(exp)), env);
                        setcdr(pair, newval);
                        return newval;
                }
                if(first == LetS) {
                        if(exp->len < 2)
                                RECOVER(l, "'let \"argc < 2 in %S\"", exp);
                        tmp = exp;
                        for(; !isnil(cdr(exp)); exp = cdr(exp)) {
                                if(!iscons(car(exp)) || !cklen(car(exp), 2))
                                   RECOVER(l, "'let \"expected list of length 2: got '%S in '%S\"",
                                                   car(exp), tmp);
                                env = extend(l, env, car(car(exp)),
                                        eval(l, depth + 1, car(cdr(car(exp))), env));
                        }
                        return eval(l, depth+1, car(exp), env);
                }
                if(first == Begin) {
                        if(isnil(exp)) return Nil;
                        for(;;) {
                                if(cdr(exp) == Nil) {
                                      exp = car(exp);
                                      goto tail;
                                }
                                eval(l, depth + 1, car(exp), env);
                                exp = cdr(exp);
                        }
                }
                proc = eval(l, depth + 1, first, env);
                if(isproc(proc) || issubr(proc)) vals = evlis(l, depth + 1, exp, env);
                else if(isfproc(proc)) vals = cons(l, exp, Nil);
                else RECOVER(l, "'not-a-procedure '%s", first);
                if(issubr(proc)) {
                        l->gc_used = gc_point;
                        gc_add(l, proc);
                        gc_add(l, vals);
                        return (*subrval(proc))(l, vals);
                }
                if(isproc(proc) || isfproc(proc)) {
                        if(procargs(proc)->len != vals->len)
                                RECOVER(l, "'proc \"expected\" %S \"got\" '%S", 
                                                procargs(proc), vals);
                        if(procargs(proc)->len)
                                env = multiple_extend(l, 
                                        l->dynamic ? env : procenv(proc), 
                                        procargs(proc), vals);
                        exp = cons(l, Begin, proccode(proc));
                        goto tail;
                }
                RECOVER(l, "'not-a-procedure '%S", car(exp));
        case INVALID: 
        default:     HALT(l, "%s", "internal inconsistency: unknown type");
        }
        HALT(l, "%s", "internal inconsistency: reached the unreachable");
        return exp; 
}

static cell *evlis(lisp *l, unsigned depth, cell *exps, cell *env) {
        size_t i;
        cell *op, *head;
        if(isnil(exps)) return Nil;
        op = car(exps);
        exps = cdr(exps);
        head = op = cons(l, eval(l, depth+1, op, env), Nil);
        for(i = 1; !isnil(exps); exps = cdr(exps), op = cdr(op), i++)
                setcdr(op, cons(l, eval(l, depth+1, car(exps), env), Nil));
        head->len = i;
        return head;
}

/******************* built in or "primitive" functions ************************/

static cell *subr_band(lisp *l, cell *args) {
        if(!cklen(args, 2) || !isint(car(args)) || !isint(car(cdr(args))))
                RECOVER(l, "\"expected (int int)\" '%S", args);
        return mkint(l, intval(car(args)) & intval(car(cdr(args))));
}

static cell *subr_bor(lisp *l, cell *args) {
        if(!cklen(args, 2) || !isint(car(args)) || !isint(car(cdr(args))))
                RECOVER(l, "\"expected (int int)\" '%S", args);
        return mkint(l, intval(car(args)) | intval(car(cdr(args))));
}

static cell *subr_bxor(lisp *l, cell *args) {
        if(!cklen(args, 2) || !isint(car(args)) || !isint(car(cdr(args))))
                RECOVER(l, "\"expected (int int)\" '%S", args);
        return mkint(l, intval(car(args)) ^ intval(car(cdr(args))));
}

static cell *subr_binv(lisp *l, cell *args) {
        if(!cklen(args, 1) || !isint(car(args)))
                RECOVER(l, "\"expected (int)\" '%S", args);
        return mkint(l, ~intval(car(args)));
}

static cell *subr_binlog(lisp *l, cell *args) {
        if(!cklen(args, 1) || !isint(car(args)))
                RECOVER(l, "\"expected (int)\" '%S", args);
        return mkint(l, binlog(intval(car(args))));
}

static cell *subr_sum(lisp *l, cell *args) {
        if(!cklen(args, 2)) 
                RECOVER(l, "\"argument count not equal 2\" '%S", args);
        cell *x = car(args), *y = car(cdr(args));
        if(isint(x) && isarith(y)) {
                if(isfloat(y)) return mkint(l, intval(x) + floatval(y));
                else           return mkint(l, intval(x) + intval(y));
        } else if(isfloat(x) && isarith(y)) {
                if(isfloat(y)) return mkfloat(l, floatval(x) + floatval(y));
                else           return mkfloat(l, floatval(x) + (lfloat) intval(y));
        }
        RECOVER(l, "\"type check problem\" %S", args);
        return Error;
}

static cell *subr_sub(lisp *l, cell *args) {
        if(!cklen(args, 2)) 
                RECOVER(l, "\"argument count not equal 2\" '%S", args);
        cell *x = car(args), *y = car(cdr(args));
        if(isint(x) && isarith(y)) {
                if(isfloat(y)) return mkint(l, intval(x) - floatval(y));
                else           return mkint(l, intval(x) - intval(y));
        } else if(isfloat(x) && isarith(y)) {
                if(isfloat(y)) return mkfloat(l, floatval(x) - floatval(y));
                else           return mkfloat(l, floatval(x) - (lfloat) intval(y));
        }
        RECOVER(l, "\"type check failed\" '%S", args);
        return Error; 
}

static cell *subr_prod(lisp *l, cell *args) {
        if(!cklen(args, 2)) 
                RECOVER(l, "\"argument count not equal 2\" '%S", args);
        cell *x = car(args), *y = car(cdr(args));
        if(isint(x) && isarith(y)) {
                if(isfloat(y)) return mkint(l, intval(x) * floatval(y));
                else           return mkint(l, intval(x) * intval(y));
        } else if(isfloat(x) && isarith(y)) {
                if(isfloat(y)) return mkfloat(l, floatval(x) * floatval(y));
                else           return mkfloat(l, floatval(x) * (lfloat) intval(y));
        }
        RECOVER(l, "\"type check failed\" '%S", args);
        return Error;
}

static cell *subr_mod(lisp *l, cell *args) {
        intptr_t dividend, divisor;
        if(!cklen(args, 2) || !isint(car(args)) || !isint(car(cdr(args))))
                RECOVER(l, "\"argument count not equal 2\" '%S", args);
        dividend = intval(car(args));
        divisor  = intval(car(cdr(args)));
        if(!divisor || (dividend == INTPTR_MIN && divisor == -1)) 
                RECOVER(l, "\"invalid divisor values\" '%S", args);
        return mkint(l, dividend % divisor);
}

static cell *subr_div(lisp *l, cell *args) {
        if(!cklen(args, 2))
                RECOVER(l, "\"argument count not equal 2\" '%S", args);
        if(isint(car(args)) && isarith(car(cdr(args)))) {
                intptr_t dividend, divisor;
                dividend = intval(car(args));
                divisor = isfloat(car(cdr(args))) ? 
                        floatval(car(cdr(args))) : 
                        intval(car(cdr(args)));
                if(!divisor || (dividend == INTPTR_MIN && divisor == -1))
                        RECOVER(l, "\"invalid divisor values\" '%S", args);
                return mkint(l, dividend / divisor);
        } else if(isfloat(car(args)) && isarith(car(cdr(args)))) {
                lfloat dividend, divisor;
                dividend = floatval(car(args));
                divisor = isfloat(car(cdr(args))) ? 
                        floatval(car(cdr(args))) : 
                        intval(car(cdr(args)));
                if(divisor == 0.f)
                        RECOVER(l, "\"division by zero in %S\"", args);
                return mkfloat(l, dividend / divisor);
        }
        RECOVER(l, "\"type check failed\" '%S", args);
        return Error;
}

static cell *subr_greater(lisp *l, cell *args) {
        cell *x, *y;
        if(!cklen(args, 2))
                RECOVER(l, "\"expected (number number) or (string string)\" '%S", args);
        x = car(args);
        y = car(cdr(args));
        if(isarith(x) && isarith(y))
                return  (isfloat(x) ? floatval(x) : intval(x)) > 
                        (isfloat(y) ? floatval(y) : intval(y)) ? Tee : Nil;
        else if(isasciiz(x) && isasciiz(x))
                return (strcmp(strval(x), strval(y)) > 0) ? Tee : Nil;
        RECOVER(l, "\"expected (number number) or (string string)\" '%S", args);
        return Error;
}

static cell *subr_less(lisp *l, cell *args) {
        cell *x, *y;
        if(!cklen(args, 2))
                RECOVER(l, "\"expected (number number) or (string string)\" '%S", args);
        x = car(args);
        y = car(cdr(args));
        if(isarith(x) && isarith(y))
                return  (isfloat(x) ? floatval(x) : intval(x)) < 
                        (isfloat(y) ? floatval(y) : intval(y)) ? Tee : Nil;
        else if(isasciiz(x) && isasciiz(x))
                return (strcmp(strval(x), strval(y)) < 0) ? Tee : Nil;
        RECOVER(l, "\"expected (number number) or (string string)\" '%S", args);
        return Error;
}

static cell *subr_eq(lisp *l, cell *args) {
        cell *x, *y;
        if(!cklen(args, 2))
                RECOVER(l, "'arg-count \"argc != 2 in %S\"", args);
        x = car(args);
        y = car(cdr(args));
        if(isuserdef(x) && l->ufuncs[x->userdef].equal)
                return (l->ufuncs[x->userdef].equal)(x, y) ? Tee : Nil;
        if(intval(x) == intval(y))
                return Tee;
        if(isstr(x) && isstr(y)) {
                if(!strcmp(strval(x), strval(y))) return Tee;
                else return Nil;
        }
        return Nil;
}

static cell *subr_cons(lisp *l, cell *args) { 
        if(!cklen(args, 2))
                RECOVER(l, "\"expected (expr expr)\" '%S", args);
        return cons(l, car(args), car(cdr(args))); 
}

static cell *subr_car(lisp *l, cell *args) { 
        if(!cklen(args, 1) || !iscons(car(args)))
                RECOVER(l, "\"expect (list)\" '%S", args);
        return car(car(args)); 
}

static cell *subr_cdr(lisp *l, cell *args) { 
        if(!cklen(args, 1) || !iscons(car(args)))
                RECOVER(l, "\"argument count not equal 1 or not a list\" '%S", args);
        return cdr(car(args)); 
}

static cell *subr_list(lisp *l, cell *args) {
        size_t i;
        cell *op, *head;
        if(cklen(args, 0))
                RECOVER(l, "\"argument count must be more than 0\" '%S", args);
        op = car(args);
        args = cdr(args);
        head = op = cons(l, op, Nil);
        for(i = 1; !isnil(args); args = cdr(args), op = cdr(op), i++)
                setcdr(op, cons(l, car(args), Nil));
        head->len = i;
        return head;
}

static cell *subr_match(lisp *l, cell *args) { 
        if(!cklen(args, 2) 
        || !isasciiz(car(args)) || !isasciiz(car(cdr(args)))) 
                RECOVER(l, "\"expected (string string)\" '%S", args);
        return match(symval(car(args)), symval(car(cdr(args)))) ? Tee : Nil; 
}

static cell *subr_scons(lisp *l, cell *args) {
        char *ret;
        if(!cklen(args, 2) 
        || !isasciiz(car(args)) || !isasciiz(car(cdr(args))))
                RECOVER(l, "\"expected (string string)\" '%S", args);
        ret = CONCATENATE(strval(car(args)), strval(car(cdr(args))));
        return mkstr(l, ret);
}

static cell *subr_scar(lisp *l, cell *args) {
        char c[2] = {'\0', '\0'};
        if(!cklen(args, 1) || !isasciiz(car(args)))
                RECOVER(l, "\"expected (string-or-symbol)\" '%S", args);
        c[0] = strval(car(args))[0];
        return mkstr(l, lstrdup(c));
}


static cell *subr_scdr(lisp *l, cell *args) {
        if(!cklen(args, 1) || !isasciiz(car(args)))
                RECOVER(l, "\"expected (string-or-symbol)\" '%S", args);
        if(!(strval(car(args))[0])) mkstr(l, lstrdup(""));
        return mkstr(l, lstrdup(&strval(car(args))[1]));;
}

static cell *subr_eval(lisp *l, cell *args) { /**XXX: allows unlimited recursion!**/
        if(cklen(args, 1)) return eval(l, 0, car(args), l->top_env);
        if(cklen(args, 2)) return eval(l, 0, car(args), car(cdr(args)));
        RECOVER(l, "\"expected (expr) or (expr environment)\" '%S", args);
        return Error;
}

static cell *subr_trace(lisp *l, cell *args) {
        if(cklen(args, 1)) {
                if(isint(car(args))) {
                        switch(intval(car(args))) {
                        case TRACE_OFF:    l->trace = TRACE_OFF; break;
                        case TRACE_MARKED: l->trace = TRACE_MARKED; break;
                        case TRACE_ALL:    l->trace = TRACE_ALL; break;
                        default: RECOVER(l, "\"invalid trace level\" '%S", car(args));
                        }
                } 
                else RECOVER(l, "\"expected (int)\" '%S", args);
        }
        return mkint(l, l->trace);
}

static cell *subr_trace_cell(lisp *l, cell *args) {
        if(cklen(args, 1)) {
                return (car(args)->trace) ? Tee : Nil;
        } else if (cklen(args, 2)) {
                if(isnil(car(cdr(args)))) {
                        car(args)->trace = 0;
                        return Nil;
                } else if(car(cdr(args)) == Tee) {
                        car(args)->trace = 1;
                        return Tee;
                } 
        } 
        RECOVER(l, "\"expected (cell) or (cell t-or-nil)\", '%S", args);
        return Error;
}

static cell *subr_gc(lisp *l, cell *args) {
        if(cklen(args, 0))
                gc_collect(l);
        if(cklen(args, 1) && isint(car(args))) {
                switch(intval(car(args))) {
                case GC_ON:       if(l->gc_state == GC_OFF) goto fail;
                                  else l->gc_state = GC_ON;
                                  break;
                case GC_POSTPONE: if(l->gc_state == GC_OFF) goto fail;
                                  else l->gc_state = GC_POSTPONE; 
                                  break;
                case GC_OFF:      l->gc_state = GC_OFF;      break;
                default: RECOVER(l, "\"invalid GC option\" '%S", args);
                }
        }
        return mkint(l, l->gc_state);
fail:   RECOVER(l, "\"garbage collection permanently off\" '%S", args);
        return Error;
}

static cell *subr_length(lisp *l, cell *args) {
        if(!cklen(args, 1)) RECOVER(l, "\"argument count is not 1\" '%S", args);
        return mkint(l, (intptr_t)car(args)->len);
}

static cell* subr_inp(lisp *l, cell *args) {
        if(!cklen(args, 1)) RECOVER(l, "\"argument count is not 1\" '%S", args);
        return isin(car(args)) ? Tee : Nil;
}

static cell* subr_outp(lisp *l, cell *args) {
        if(!cklen(args, 1)) RECOVER(l, "\"argument count is not 1\" '%S", args);
        return isout(car(args)) ? Tee : Nil;
}

static cell* subr_open(lisp *l, cell *args) {
        io *ret;
        char *file;
        if(!cklen(args, 2) || !isint(car(args)) || !isstr(car(cdr(args)))) 
                RECOVER(l, "\"expected (integer string)\" '%S", args);
        file = strval(car(cdr(args)));
        switch(intval(car(args))) {
        case FIN:  ret = io_fin(fopen(file, "rb")); break;
        case FOUT: ret = io_fout(fopen(file, "wb")); break;
        case SIN:  ret = io_sin(file); break;
        /*case SOUT: will not be implemented.*/
        default:   RECOVER(l, "\"invalid operation %d\" '%S", intval(car(args)), args);
        }
        return ret == NULL ? Nil : mkio(l, ret);
}

static cell* subr_getchar(lisp *l, cell *args) {

        if(cklen(args, 0)) return mkint(l, io_getc(l->ifp));
        if(cklen(args, 1) && isin(car(args)))
                return mkint(l, io_getc(ioval(car(args))));
        RECOVER(l, "\"expected () or (input)\" '%S", args);
        return Error;
}

static cell* subr_getdelim(lisp *l, cell *args) {
        char *s;
        if(cklen(args, 1) && isstr(car(args)))
                return (s = io_getdelim(l->ifp, strval(car(args))[0])) ? 
                                mkstr(l, s) : Nil;
        if(cklen(args, 2) && isin(car(args)) && isstr(car(cdr(args))))
                return (s = io_getdelim(ioval(car(args)), strval(car(cdr(args)))[0])) ? 
                                mkstr(l, s) : Nil;
        RECOVER(l, "\"expected (string) or (input string)\" '%S", args);
        return Error;
}

static cell* subr_read(lisp *l, cell *args) {
        cell *ob = NULL;
        int restore_used, r;
        jmp_buf restore;
        if(l->recover_init) {
                memcpy(restore, l->recover, sizeof(jmp_buf));
                restore_used = 1;
        }
        l->recover_init = 1;
        if((r = setjmp(l->recover))) { 
                if(restore_used) memcpy(l->recover, restore, sizeof(jmp_buf));
                else             l->recover_init = 0;
                return Error;
        }

        if(cklen(args, 0))
                (ob = reader(l, l->ifp)) ? ob : Error;
        if(cklen(args, 1) && isin(car(args)))
                (ob = reader(l, ioval(car(args)))) ? ob : Error;
        if(restore_used) memcpy(l->recover, restore, sizeof(jmp_buf));
        else             l->recover_init = 0;
        if(!ob) RECOVER(l, "\"expected () or (input)\" '%S", args);
        return ob;
}

static cell* subr_puts(lisp *l, cell *args) {
        if(cklen(args, 1) && isstr(car(args)))
                return io_puts(strval(car(args)),l->ofp) < 0 ? Nil : car(args);
        if(cklen(args, 2) && isout(car(args)) && isstr(car(cdr(args))))
                return io_puts(strval(car(cdr(args))), ioval(car(args))) < 0 ?
                        Nil : car(cdr(args));
        RECOVER(l, "\"expected (string) or (output string)\" '%S", args);
        return Error;
}

static cell* subr_putchar(lisp *l, cell *args) {
        if(cklen(args, 1) && isint(car(args)))
                return io_putc(intval(car(args)),l->ofp) < 0 ? Nil : car(args);
        if(cklen(args, 2) && isout(car(args)) && isint(car(cdr(args))))
                return io_putc(intval(car(args)), ioval(car(cdr(args)))) < 0 ?
                        Nil : car(cdr(args));
        RECOVER(l, "\"expected (integer) or (output integer)\" '%S", args);
        return Error;
}

static cell* subr_print(lisp *l, cell *args) {
        if(cklen(args, 1)) 
                return printer(l->ofp, car(args), 0) < 0 ? Nil : car(args); 
        if(cklen(args, 2) && isout(car(args))) 
                return printer(ioval(car(args)), car(cdr(args)), 0) < 0 ? 
                        Nil : car(cdr(args)); 
        RECOVER(l, "\"expected (expr) or (output expression)\" '%S", args);
        return Error;
}

static cell* subr_flush(lisp *l, cell *args) {
        if(cklen(args, 0)) return mkint(l, fflush(NULL));
        if(cklen(args, 1) && isio(car(args))) 
                return io_flush(ioval(car(args))) ? Nil : Tee;
        RECOVER(l, "\"expected () or (io)\" '%S", args);
        return Error;
}

static cell* subr_tell(lisp *l, cell *args) {
        if(cklen(args, 1) && isio(car(args)))
                return mkint(l, io_tell(ioval(car(args))));
        RECOVER(l, "\"expected (io)\" '%S", args);
        return Error;
}

static cell* subr_seek(lisp *l, cell *args) { /*XXX: accepts invalid enums*/
        if(cklen(args, 3) && isio(car(args)) 
                && isint(car(cdr(args))) && isint(car(cdr(cdr(args))))) {
                switch (intval(car(cdr(args)))) {
                case SEEK_SET: case SEEK_CUR: case SEEK_END: break;
                default: RECOVER(l, "\"invalid enum option\" '%S", args);
                }
                return mkint(l,io_seek(ioval(car(args)),intval(car(cdr(args))),
                                        intval(car(cdr(cdr(args))))));
        }
        RECOVER(l, "\"expected (io integer integer)\" '%S", args);
        return Error;
}

static cell* subr_eofp(lisp *l, cell *args) {
        if(cklen(args, 1) && isio(car(args)))
                return io_eof(ioval(car(args))) ? Tee : Nil;
        RECOVER(l, "\"expected (io)\" '%S", args);
        return Error;
}

static cell* subr_ferror(lisp *l, cell *args) {
        if(cklen(args, 1) && isio(car(args)))
                return io_error(ioval(car(args))) ? Tee : Nil;
        RECOVER(l, "\"expected (io)\" '%S", args);
        return Error;
}

static cell* subr_system(lisp *l, cell *args) {
        if(cklen(args, 0))
                return mkint(l, system(NULL));
        if(cklen(args, 1) && isasciiz(car(args)))
                return mkint(l, system(strval(car(args))));
        RECOVER(l, "\"expected () or (string)\" '%S", args);
        return Error;
}

static cell* subr_remove(lisp *l, cell *args) {
        if(!cklen(args, 1) || !isasciiz(car(args)))
                RECOVER(l, "\"expected (string)\" '%S", args);
        return remove(strval(car(args))) ? Nil : Tee ;
}

static cell* subr_rename(lisp *l, cell *args) {
        if(!cklen(args, 2) 
        || !isasciiz(car(args)) || !isasciiz(car(cdr(args)))) 
                RECOVER(l, "\"expected (string string)\" '%S", args);
        return rename(strval(car(args)), strval(car(cdr(args)))) ? Nil : Tee;
}

static cell* subr_allsyms(lisp *l, cell *args) {
        UNUSED(args);
        return l->all_symbols;
}

static cell* subr_hlookup(lisp *l, cell *args) {
        cell *ob;
        if(!cklen(args, 2) || !ishash(car(args)) || !isasciiz(car(cdr(args))))
                RECOVER(l, "\"expected (hash symbol-or-string)\" %S", args);
        return (ob = hash_lookup(hashval(car(args)),
                                symval(car(cdr(args))))) ? ob : Nil; 
}

static cell* subr_hinsert(lisp *l, cell *args) {
        if(!cklen(args, 3) || !ishash(car(args)) || !issym(car(cdr(args))))
                RECOVER(l, "\"expected (hash symbol expression)\" %S", args);
        if(hash_insert(hashval(car(args)), 
                        symval(car(cdr(args))), car(cdr(cdr(args)))))
                                HALT(l, "%s", "out of memory");
        return car(args); 
}

static cell* subr_hcreate(lisp *l, cell *args) { 
        hashtable *ht;
        if(args->len % 2)
                RECOVER(l, "\"expected even number of arguments\" '%S", args);
        if(!(ht = hash_create(DEFAULT_LEN))) HALT(l, "%s", "out of memory");
        for(;!isnil(args); args = cdr(cdr(args))) {
                if(!isasciiz(car(args))) return Error;
                hash_insert(ht, symval(car(args)), car(cdr(args)));
        }
        return mkhash(l, ht); 
}

static cell* subr_coerce(lisp *l, cell *args) { 
        char *fltend = NULL;
        intptr_t d = 0;
        size_t i = 0, j;
        cell *convfrom, *x, *y, *head;
        if(!cklen(args, 2) && issym(car(args))) goto fail;
        convfrom = car(cdr(args));
        if(intval(car(args)) == convfrom->type) return convfrom;
        switch(intval(car(args))) {
        case INTEGER: 
                    if(isstr(convfrom)) { /*int to string*/
                            if(!isnumber(strval(convfrom))) goto fail;
                            sscanf(strval(convfrom), "%"SCNiPTR, &d);
                    }
                    if(isfloat(convfrom)) /*float to string*/
                           d = (intptr_t) floatval(convfrom);
                    return mkint(l, d);
        case CONS:  if(isstr(convfrom)) { /*string to list of chars*/
                            head = x = cons(l, Nil, Nil);
                            for(i = 0; i < convfrom->len; i++) {
                                char c[2] = {'\0', '\0'};
                                c[0] = strval(convfrom)[i];
                                y = mkstr(l, lstrdup(c));
                                setcdr(x, cons(l, y, Nil));
                                x = cdr(x);
                            }
                            cdr(head)->len = i;
                            return cdr(head);
                    }
                    if(ishash(convfrom)) { /*hash to list*/
                            hashentry *cur;
                            hashtable *h = hashval(convfrom);
                            head = x = cons(l, Nil, Nil);
                            for(j = 0, i = 0; i < h->len; i++)
                                    if(h->table[i])
                                            for(cur = h->table[i]; cur; cur = cur->next, j++) {
                                                    y = mkstr(l, lstrdup(cur->key));
                                                    setcdr(x, cons(l, y, Nil));
                                                    x = cdr(x);
                                                    setcdr(x, cons(l, (cell*)cur->val, Nil));
                                                    x = cdr(x);
                                            }
                            cdr(head)->len = j;
                            return cdr(head);
                    }
                    break;
        case STRING:if(isint(convfrom)) { /*int to string*/
                            char s[64] = "";
                            sprintf(s, "%"PRIiPTR, intval(convfrom));
                            return mkstr(l, lstrdup(s));
                    }
                    if(issym(convfrom)) /*symbol to string*/
                           return mkstr(l, lstrdup(strval(convfrom)));
                    if(isfloat(convfrom)) { /*float to string*/
                            char s[512] = "";
                            sprintf(s, "%f", floatval(convfrom));
                            return mkstr(l, lstrdup(s));
                    }
                    break;
        case SYMBOL:if(isstr(convfrom) && !strpbrk(strval(convfrom), " \t\n\r'\"\\"))
                            return intern(l, lstrdup(strval(convfrom)));
                    break;
        case HASH:  if(iscons(convfrom)) /*hash from list*/
                            return subr_hcreate(l, convfrom);
                    break;
        case FLOAT: if(isint(convfrom)) /*int to float*/
                          return mkfloat(l, intval(convfrom));
                    if(isstr(convfrom)) { /*string to float*/
                          lfloat d;
                          if(!isfnumber(strval(convfrom))) goto fail;
                          d = strtod(strval(convfrom), &fltend);
                          if(!fltend[0]) return mkfloat(l, d);
                          else           goto fail;
                    }
        default: break;
        }
fail:   RECOVER(l, "\"invalid conversion or argument length not 2\" %S", args);
        return Error;
}

static cell* subr_time(lisp *l, cell *args) {
        UNUSED(args);
        return mkint(l, time(NULL));
}

static cell* subr_date(lisp *l, cell *args) { /*not thread safe, also only GMT*/
        time_t raw;
        struct tm *gt;
        UNUSED(args);
        time(&raw);
        gt = gmtime(&raw);
        return cons(l,  mkint(l, gt->tm_year + 1900),
                cons(l, mkint(l, gt->tm_mon + 1),
                cons(l, mkint(l, gt->tm_mday),
                cons(l, mkint(l, gt->tm_hour),
                cons(l, mkint(l, gt->tm_min),
                cons(l, mkint(l, gt->tm_sec), Nil))))));
}

static cell* subr_getenv(lisp *l, cell *args) {
        char *ret;
        if(!cklen(args, 1) || !isstr(car(args)))
                RECOVER(l, "\"expected (string)\" '%S", args);
        return (ret = getenv(strval(car(args)))) ? mkstr(l, lstrdup(ret)) : Nil;
}

static cell *subr_rand(lisp *l, cell *args) {
        UNUSED(args);
        return mkint(l, xorshift128plus(l->random_state));
}

static cell *subr_seed(lisp *l, cell *args) {
        if(!cklen(args, 2) || !isint(car(args)) || !isint(car(cdr(args))))
                RECOVER(l, "\"expected (integer integer)\" %S", args);
        l->random_state[0] = intval(car(args));
        l->random_state[1] = intval(car(cdr(args)));
        return Tee;
}

static cell* subr_assoc(lisp *l, cell *args) {
        if(!cklen(args, 2) || !iscons(car(cdr(args))))
                RECOVER(l, "\"expected (val a-list)\" '%S", args);
        return assoc(car(args), car(cdr(args)));
}

static cell *subr_setlocale(lisp *l, cell *args) {
        char *ret = NULL; /*this function is not reentrant, also should have more options*/
        if(!cklen(args, 2) || !isint(car(args)) || !isasciiz(car(cdr(args))))
                RECOVER(l, "\"expected (int string-or-symbol)\" '%S", args);
        switch(intval(car(args))) {
        case LC_ALL:      case LC_COLLATE: case LC_CTYPE:
        case LC_MONETARY: case LC_NUMERIC: case LC_TIME:
                ret = setlocale(intval(car(args)), strval(car(cdr(args))));
                break;
        default: RECOVER(l, "\"invalid int value\" '%S", args);
        }
        if(!ret) return Nil; /*failed to set local*/
        return mkstr(l, lstrdup(ret));
}

static cell *subr_typeof(lisp *l, cell *args) {
        if(!cklen(args, 1))
                RECOVER(l, "\"expected (expr)\" %S", args);
        return mkint(l, car(args)->type);
}

static cell *subr_close(lisp *l, cell *args) {
        UNUSED(l); UNUSED(args);
        /**XXX: implement this, close an IO object**/
        return Error;
}

/*X-Macro of primitive functions and their names; basic built in subr*/
#define SUBR_LIST\
        X(subr_band,    "&")            X(subr_bor,      "|")\
        X(subr_bxor,    "^")            X(subr_binv,     "~")\
        X(subr_sum,     "+")            X(subr_sub,      "-")\
        X(subr_prod,    "*")            X(subr_mod,      "%")\
        X(subr_div,     "/")            X(subr_eq,       "=")\
        X(subr_eq,      "eq")           X(subr_greater,  ">")\
        X(subr_less,    "<")            X(subr_cons,     "cons")\
        X(subr_car,     "car")          X(subr_cdr,      "cdr")\
        X(subr_list,    "list")         X(subr_match,    "match")\
        X(subr_scons,   "scons")        X(subr_scar,     "scar")\
        X(subr_scdr,    "scdr")         X(subr_eval,     "eval")\
        X(subr_trace,   "trace-level!") X(subr_gc,       "gc")\
        X(subr_length,  "length")       X(subr_typeof,   "type-of")\
        X(subr_inp,     "input?")       X(subr_outp,     "output?")\
        X(subr_eofp,    "eof?")         X(subr_flush,    "flush")\
        X(subr_tell,    "tell")         X(subr_seek,     "seek")\
        X(subr_close,   "close")        X(subr_open,     "open")\
        X(subr_getchar, "get-char")     X(subr_getdelim, "get-delim")\
        X(subr_read,    "read")         X(subr_puts,     "put")\
        X(subr_putchar, "put-char")     X(subr_print,    "print")\
        X(subr_ferror,  "ferror")       X(subr_system,   "system")\
        X(subr_remove,  "remove")       X(subr_rename,   "rename")\
        X(subr_allsyms, "all-symbols")  X(subr_hcreate,  "hash-create")\
        X(subr_hlookup, "hash-lookup")  X(subr_hinsert,  "hash-insert")\
        X(subr_coerce,  "coerce")       X(subr_time,     "time")\
        X(subr_getenv,  "getenv")       X(subr_rand,     "random")\
        X(subr_seed,    "seed")         X(subr_date,     "date")\
        X(subr_assoc,   "assoc")        X(subr_setlocale, "locale!")\
        X(subr_trace_cell, "trace")     X(subr_binlog,    "binary-logarithm")\
        

#define X(SUBR, NAME) { SUBR, NAME },
static struct subr_list { subr p; char *name; } primitives[] = {
        SUBR_LIST  /*all of the subr*/
        {NULL, NULL} /*must be terminated with NULLs*/
};
#undef X

#define INTEGER_LIST\
        X("*seek-cur*",     SEEK_CUR)     X("*seek-set*",    SEEK_SET)\
        X("*seek-end*",     SEEK_END)     X("*random-max*",  INTPTR_MAX)\
        X("*integer-max*",  INTPTR_MAX)   X("*integer-min*", INTPTR_MIN)\
        X("*integer*",      INTEGER)      X("*symbol*",      SYMBOL)\
        X("*cons*",         CONS)         X("*string*",      STRING)\
        X("*hash*",         HASH)         X("*io*",          IO)\
        X("*float*",        FLOAT)        X("*procedure*",   PROC)\
        X("*primitive*",    SUBR)         X("*f-procedure*", FPROC)\
        X("*file-in*",      FIN)          X("*file-out*",    FOUT)\
        X("*string-in*",    SIN)          X("*string-out*",  SOUT)\
        X("*lc-all*",       LC_ALL)       X("*lc-collate*",  LC_COLLATE)\
        X("*lc-ctype*",     LC_CTYPE)     X("*lc-monetary*", LC_MONETARY)\
        X("*lc-numeric*",   LC_NUMERIC)   X("*lc-time*",     LC_TIME)\
        X("*user-defined*", USERDEF)      X("*trace-off*",   TRACE_OFF)\
        X("*trace-marked*", TRACE_MARKED) X("*trace-all*",   TRACE_ALL)\
        X("*gc-on*",        GC_ON)        X("*gc-postpone*", GC_POSTPONE)\
        X("*gc-off*",       GC_OFF)

#define X(NAME, VAL) { NAME, VAL }, 
static struct integer_list { char *name; intptr_t val; } integers[] = {
        INTEGER_LIST
        {NULL, 0}
};
#undef X

/***************** initialization and lisp interfaces *************************/

cell *lisp_add_subr(lisp *l, subr func, char *name) { assert(l && func && name);
        return extend_top(l, intern(l, lstrdup(name)), mksubr(l, func));
}

cell *lisp_intern(lisp *l, cell *ob) { assert(l && ob);
        if(hash_insert(hashval(l->all_symbols), symval(ob) , ob)) return NULL;
        return Tee;
}

cell *lisp_add_cell(lisp *l, char *sym, cell *val) { assert(l && sym && val);
        return extend_top(l, intern(l, lstrdup(sym)), val);
}

void lisp_destroy(lisp *l) {       
        if(!l) return;
        if(l->gc_stack) gc(l), free(l->gc_stack);
        if(l->buf) free(l->buf);
        if(l->efp) io_close(l->efp);
        if(l->ofp) io_close(l->ofp);
        if(l->ifp) io_close(l->ifp);
        free(l);
}

lisp *lisp_init(void) {
        lisp *l;
        unsigned i;
        if(!(l = calloc(1, sizeof(*l))))       goto fail;
        if(!(l->ifp = io_fin(stdin)))          goto fail;
        if(!(l->ofp = io_fout(stdout)))        goto fail;
        if(!(l->efp = io_fout(stderr)))        goto fail;
        if(!(l->buf = calloc(DEFAULT_LEN, 1))) goto fail;
        l->buf_allocated = DEFAULT_LEN;
        if(!(l->gc_stack = calloc(DEFAULT_LEN, sizeof(*l->gc_stack)))) 
                goto fail;
        l->gc_allocated = DEFAULT_LEN;
        l->max_depth = 4096; /*max recursion depth*/

        /*I should probably discard the first N random numbers*/
        l->random_state[0] = 0xCAFE; /*Are these good seeds?*/
        l->random_state[1] = 0xBABE; /*???*/

        if(!(l->all_symbols = mkhash(l, hash_create(4096)))) goto fail;
        if(!(l->top_env = cons(l, cons(l, Nil, Nil), Nil))) goto fail;

        /*it would be nice if this was all statically allocated*/
        for(i = 0; special_cells[i].internal; i++) /*add special cells*/
                if(!lisp_intern(l, special_cells[i].internal))
                        goto fail;

        if(!extend_top(l, Tee, Tee)) goto fail;

        if(!lisp_add_cell(l, "pi", mkfloat(l, 3.14159265358979323846))) goto fail;
        if(!lisp_add_cell(l, "e",  mkfloat(l, 2.71828182845904523536))) goto fail;

        for(i = 0; integers[i].name; i++) /*add all integers*/
                if(!lisp_add_cell(l, integers[i].name, mkint(l, integers[i].val)))
                        goto fail;
        for(i = 0; primitives[i].p; i++) /*add all primitives*/
                if(!lisp_add_subr(l, primitives[i].p, primitives[i].name))
                        goto fail;
        return l;
fail:   lisp_destroy(l);
        return NULL;
}

cell *lisp_read(lisp *l, io *i) { assert(l && i);
        cell *ret;
        int restore_used, r;
        jmp_buf restore;
        if(l->recover_init) {
                memcpy(restore, l->recover, sizeof(jmp_buf));
                restore_used = 1;
        }
        if((r = setjmp(l->recover))) { 
                if(restore_used) memcpy(l->recover, restore, sizeof(jmp_buf));
                else             l->recover_init = 0;
                return r > 0 ? Error : NULL;
        }
        l->recover_init = 1;
        ret = reader(l, i);
        if(restore_used) memcpy(l->recover, restore, sizeof(jmp_buf));
        else             l->recover_init = 0;
        return ret;
}

int lisp_print(lisp *l, cell *ob) { assert(l && ob);
        int ret = printer(l->ofp, ob, 0);
        io_putc('\n', l->ofp);
        return ret;
}

cell *lisp_eval(lisp *l, cell *exp) { assert(l && exp);
        cell *ret;
        int restore_used, r;
        jmp_buf restore;
        if(l->recover_init) {
                memcpy(restore, l->recover, sizeof(jmp_buf));
                restore_used = 1;
        }
        if((r = setjmp(l->recover))) {
                if(restore_used) memcpy(l->recover, restore, sizeof(jmp_buf));
                else             l->recover_init = 0;
                return r > 0 ? Error : NULL;
        }
        l->recover_init = 1;
        ret = eval(l, 0, exp, l->top_env);
        if(restore_used) memcpy(l->recover, restore, sizeof(jmp_buf));
        else             l->recover_init = 0;
        return ret;
}

cell *lisp_eval_string(lisp *l, char *evalme) { assert(l && evalme);
        io *in = NULL;
        cell *ret;
        volatile int restore_used = 0;
        int r;
        jmp_buf restore;
        if(l->recover_init) {
                memcpy(restore, l->recover, sizeof(jmp_buf));
                restore_used = 1;
        }
        if((r = setjmp(l->recover))) {
                io_close(in);
                if(restore_used) memcpy(l->recover, restore, sizeof(jmp_buf));
                else             l->recover_init = 0;
                return r > 0 ? Error : NULL;
        }
        l->recover_init = 1;
        if(!(in = io_sin(evalme))) return NULL;
        ret = eval(l, 0, reader(l, in), l->top_env);
        io_close(in);
        if(restore_used) memcpy(l->recover, restore, sizeof(jmp_buf));
        else             l->recover_init = 0;
        return ret;
}

int lisp_set_input(lisp *l, io *in) { assert(l && in);
        if(!io_isin(in)) return -1;
        l->ifp = in;
        return 0;
}

int lisp_set_output(lisp *l, io *out) { assert(l && out);
        if(!io_isout(out)) return -1;
        l->ofp = out;
        return 0;
}

int lisp_set_logging(lisp *l, io *logging) { assert(l && logging);
        if(!io_isout(logging)) return -1;
        l->efp = logging;
        return 0;
}

int lisp_set_line_editor(lisp *l, editor_func editor){ assert(l);
        l->editor = editor;
        return 0;
}

io *lisp_get_input(lisp *l)   { assert(l); return l->ifp; }
io *lisp_get_output(lisp *l)  { assert(l); return l->ofp; }
io *lisp_get_logging(lisp *l) { assert(l); return l->efp; }

/**************************** example program *********************************/

static lisp *lglobal;
static int running = 0; 
static char *usage = "usage: %s (-[hdcpE])* (-e string)? (-o file)? file* -\n";
enum {go_switch, go_in_file, go_out_file, go_in_string, go_error, go_in_stdin};

static void int_sig_handle(int sig) { 
        if(!running) exit(0); 
        assert(lglobal);
        lglobal->sig = sig; 
        running = 0; 
}

static int getoptions(lisp *l, char *arg, char *arg_0)
{ /**@brief simple parser for command line options**/
        int c;
        if('-' != *arg++) return go_in_file;
        if(!arg[0]) return go_in_stdin;
        while((c = *arg++))
                switch(c) {
                case 'h': printf(usage, arg_0); exit(0); break;
                case 'd': l->debug_on  = 1; break;
                case 'c': l->color_on  = 1; break;
                case 'p': l->prompt_on = 1; break;
                case 'E': l->editor_on = 1; break;
                case 'e': return go_in_string;
                case 'o': return go_out_file;
                default:  fprintf(stderr, "unknown option '%c'\n", c);
                          fprintf(stderr, usage, arg_0);
                          return go_error;
                }
        return go_switch;
}

int lisp_repl(lisp *l, char *prompt, int editor_on) {
        /**XXX: Leaks memory when an error is encountered in line editor
         *      mode. This is probably due to some weird longjmp
         *      interactions**/
        cell *ret;
        char *line = NULL;
        int r = 0;
        l->ofp->pretty = l->efp->pretty = 1; /*pretty print output*/
        l->ofp->color = l->efp->color = l->color_on;
        if((r = setjmp(l->recover)) < 0) {  /*catch errors and SIGINT*/
                l->recover_init = 0;
                running = 0;
                return r; 
        }
        l->recover_init = 1;
        running = 0;
        free(line);
        if(editor_on && l->editor) { /*handle line editing functionality*/
                while((line = l->editor(prompt))) {
                        cell *prn; 
                        if(!line[strspn(line, " \t\r\n")]) { free(line); continue; }
                        running = 1; /*SIGINT handling on*/
                        if(!(prn = lisp_eval_string(l, line)))
                                RECOVER(l, "\"invalid or incomplete line\"%s", "");
                        running = 0; /*SIGINT handling off*/
                        lisp_print(l, prn);
                        free(line);
                        line = NULL;
                }
        } else { /*read from stdin with no special handling, or a file*/
                for(;;){
                        printerf(l->ofp, 0, "%s", prompt);
                        if(!(ret = reader(l, l->ifp))) break;
                        running = 1; /*SIGINT handling on*/
                        if(!(ret = eval(l, 0, ret, l->top_env))) break;
                        running = 0; /*SIGINT handling off*/
                        printerf(l->ofp, 0, "%S\n", ret);
                        l->gc_used = 0;
                }
        }
        running = 0;
        l->gc_used = 0;
        l->recover_init = 0;
        return r;
}

int main_lisp_env(lisp *l, int argc, char **argv) {
        int i, stdin_off = 0;
        cell *ob = Nil;
        if(!l) return -1;
        lglobal = l;
        if(signal(SIGINT, int_sig_handle) == SIG_ERR) return -1;
        for(i = argc - 1; i + 1 ; i--) /*add command line args to list*/
                if(!(ob = cons(l, mkstr(l, lstrdup(argv[i])), ob))) 
                        return -1; 
        if(!extend_top(l, intern(l, lstrdup("args")), ob))
                return -1;
        for(i = 1; i < argc; i++)
                switch(getoptions(l, argv[i], argv[0])) {
                case go_switch: break;
                case go_in_stdin: /*read from standard input*/
                        io_close(l->ifp);
                        if(!(l->ifp = io_fin(stdin)))
                                return perror("stdin"), -1;
                        if(l->debug_on)
                                printerf(l->ofp, 1, 
                                          "(debug 'stdin-in)\n", argv[i]);
                        if(lisp_repl(l, l->prompt_on ? "> ": "", l->editor_on) < 0) 
                                return -1;
                        io_close(l->ifp);
                        l->ifp = NULL;
                        stdin_off = 1;
                        break;
                case go_in_file: /*read from a file*/
                        io_close(l->ifp);
                        if(!(l->ifp = io_fin(fopen(argv[i], "rb"))))
                                return perror(argv[i]), -1;
                        if(l->debug_on)
                                printerf(l->ofp, 1, 
                                          "(debug 'file-in \"%s\")\n", argv[i]);
                        if(lisp_repl(l, "", 0) < 0) return -1;
                        io_close(l->ifp);
                        l->ifp = NULL;
                        stdin_off = 1;
                        break;
                case go_in_string: /*evaluate a string*/
                        io_close(l->ifp);
                        if(!(++i < argc))
                                return fprintf(stderr, "-e expects arg\n"), -1;
                        if(!(l->ifp = io_sin(argv[i])))
                                return perror(argv[i]), -1;
                        if(l->debug_on)
                                printerf(l->ofp, 1, 
                                       "(debug 'string-in \"%s\")\n", argv[i]);
                        if(lisp_repl(l, "", 0) < 0) return -1;
                        io_close(l->ifp);
                        l->ifp = NULL;
                        stdin_off = 1;
                        break;
                case go_out_file: /*change the file to write to*/
                        if(!(++i < argc))
                                return fprintf(stderr, "-o expects arg\n"), -1;
                        if(l->debug_on)
                                printerf(l->ofp, 1, 
                                        "(debug 'file-out \"%s\")\n", argv[i]);
                        io_close(l->ofp);
                        if(!(l->ofp = io_fout(fopen(argv[i], "wb"))))
                                return perror(argv[i]), -1;
                        break;
                case go_error:
                default: exit(-1);
                }
        if(!stdin_off)
                if(lisp_repl(l, l->prompt_on ? "> ": "", l->editor_on) < 0) return -1;
        if(l->debug_on)
                printerf(l->ofp, 1, "(debug %S %S)\n", 
                                l->all_symbols, l->top_env);
        lisp_destroy(l);
        return 0;
}

int main_lisp(int argc, char **argv) {
        lisp *l;
        if(!(l = lisp_init())) return -1;
        return main_lisp_env(l, argc, argv);
}

