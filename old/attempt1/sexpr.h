/** sexpr.h */

#ifndef SEXPR_H /** header guard */
#define SEXPR_H
/**** data structures *********************************************************/
typedef enum { 
  S_NONE,     S_LIST,       S_STRING, S_SYMBOL, 
  S_INTEGER,  S_PRIMITIVE,  S_FILE,   S_POINTER,
  S_FUNCTION
} sexpr_e;

typedef enum{
  false,
  true
} bool; /** be *very* careful with this type*/

typedef struct { /** base type for our expressions */
  size_t len;
  void *buf;
  sexpr_e type;
  /** bool mark */
} sexpr_t, *expr;

typedef struct { /** linked list of all allocated memory*/
  void *alloc;
  void *next;
} alloc_t, *alloc;

typedef enum { /** enum describing all the io destinations */
  invalid_io,
  file_in,
  file_out,
  string_in,
  string_out
} iotype;

typedef union { /** pointers to where we want to write to or read from */
  FILE *file;
  char *string;
} ioptr;

typedef struct { /** I/O abstraction structure */
  iotype type;            /** what are we abstracting?*/
  ioptr ptr;              /** either FILE* or string */
  unsigned int position;  /** position in string */
  unsigned int max;       /** max string length, if known */
  char c;                 /** character store for wungetc() */
  bool ungetc;            /** true if we have ungetc'ed a character */
} io;

typedef struct{ /** a lisp environment */
  io i;                   /** input */
  io o;                   /** output */
  io e;                   /** stderr */
  expr current;           /** current s-expr */
  expr global;            /** gloabl s-expr */
} lispenv_t, *lisp;

/******************************************************************************/

/**** function prototypes *****************************************************/
void *wmalloc(size_t size, io *e);
void *wcalloc(size_t num, size_t size, io *e);
void *wrealloc(void *ptr, size_t size, io *e);
void wfree(void * ptr, io *e);

void doreport(const char *s, char *cfile, unsigned int linenum, io *e);

void append(expr list, expr ele, io *e);

expr parse_term(io *i, io *e);
void print_expr(expr x, io *o, unsigned int depth, io *e);
void free_expr(expr x, io *e);
/******************************************************************************/

/**** macros ******************************************************************/
#define BUFLEN      (256u)
#define report(X)   doreport(X,__FILE__,__LINE__,e)
#define NULLCHK(X)  if(NULL == (X)){ report("null dereference"); exit(-1);}
/******************************************************************************/

#endif
