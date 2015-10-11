(define (main)
  (driver (snarfing (read))))

(define (snarfing x)
  (cond ((eof-object? x) '())
        (#t (cons x (snarfing (read))))))

(define (driver forms)
  (driver-1 forms (map-define-name forms)))

(define (driver-1 forms procedures)
  (emit-prelude procedures)
  (driver-loop procedures forms)
  (emit-postlude))

(define (driver-loop procedures forms)
  (cond ((null? forms))
        (#t (driver-loop-1 procedures (car forms))
            (driver-loop procedures (cdr forms)))))

(define (driver-loop-1 procedures form)
  (drive-define (caadr form) (cdadr form) (cddr form) procedures))

(define (drive-define name params body procedures)
  (emit-def-proc name params)
  (comp-seq body params procedures #t)
  (emit-end-proc))

(define (map-define-name es)
  (cond ((null? es) '())
        (#t (cons (caadr (car es)) (map-define-name (cdr es))))))


(define (comp-expr expr env procedures tail-pos)
  (cond ((symbol? expr) (emit-local expr))
        ((string? expr) (comp-expr (express expr) env procedures tail-pos))
        ((==? #f (pair? expr)) (emit-literal expr))
	((same? (car expr) 'quote)
         (comp-constant (cadr expr) env procedures tail-pos))
	((same? (car expr) 'cond)
         (comp-cond (cdr expr) env procedures tail-pos))
	(#t (comp-call (car expr) (cdr expr) env procedures tail-pos))))

(define (comp-constant constant env procedures tail-pos)
  (cond ((cond ((symbol? constant) #t)
               ((string? constant) #t)
               ((pair? constant) #t)
               (#t #f))
         (comp-expr (express constant) env procedures tail-pos))
        (#t (emit-literal constant))))

(define (comp-call rator rands env procedures tail-pos)
  (comp-exprs rands env procedures)
  (cond ((primitive? rator procedures)
         (emit-prim rator (my-length rands)))
        (tail-pos (emit-call 'tailcall rator (my-length rands)))
        (#t       (emit-call 'call     rator (my-length rands)))))

(define (comp-cond clauses env procedures tail-pos)
  (cond ((null? clauses) (comp-call 'abort '() env procedures tail-pos))
        (#t (comp-expr (caar clauses) env procedures #f)
            (emit-if)
            (comp-seq (cdar clauses) env procedures tail-pos)
            (emit-else)
            (comp-cond (cdr clauses) env procedures tail-pos)
            (emit-endif))))

(define (comp-seq exprs env procedures tail-pos)
  (cond ((null? exprs) (emit-literal #f))
        ((null? (cdr exprs))
         (comp-expr (car exprs) env procedures tail-pos))
	(#t (comp-expr (car exprs) env procedures #f)
            (emit-pop)
            (comp-seq (cdr exprs) env procedures tail-pos))))

(define (comp-exprs exprs env procedures)
  (cond ((null? exprs))
	(#t (comp-expr (car exprs) env procedures #f)
            (comp-exprs (cdr exprs) env procedures))))

(define (express constant)
  (cond ((symbol? constant) (list2 'implode (express (explode constant))))
        ((string? constant) (list2 'list->string
                                   (express (string->list constant))))
        ((null? constant)   ''())
        ((pair? constant)   (list3 'cons
                                   (express (car constant))
                                   (express (cdr constant))))
        (#t constant)))

(define (primitive? name procedures)
  (==? #f (memq name procedures)))


(define (emit-prelude procedures)
  (put "
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum { heap_size  = 512*1024 };
enum { stack_size =  64*1024 };

static void die (const char *message) {
  fprintf (stderr, \"%s\\n\", message);
  exit (1);
}

typedef int obj;

#define tag(o)        ( (o) & 0x7 )
#define bits(o)       ( (o) >> 3 )

#define entag(t, ptr) ( (t) | ((ptr) << 3) )
#define mkptr(t, ptr) ( check_ptr (ptr), entag ((t), (ptr)) )
#define mkchar(n)     ( (obj) (a_char | ((n) << 3)) )
#define boolean(f)    ( (f) ? true : false )

enum { nil, eof, a_boolean, a_char, a_symbol, a_string, a_pair };
enum { false = entag (a_boolean, 0) };
enum { true  = entag (a_boolean, 1) };

#define check_ptr(p)  ( assert (0 <= (p) && (p) < heap_size) )

static obj heap[2 * heap_size];
static char marks[heap_size];
static int hp = heap_size;	/* heap pointer */

static obj stack[stack_size];
static int sp = 0;		/* stack pointer */

static void mark (obj x) {
  while (a_symbol == tag (x) || a_string == tag (x) || a_pair == tag (x)) {
    int i = bits (x);
    check_ptr (i);
    if (marks[i]) break;
    marks[i] = 1;
    mark (heap[2*i + 0]);
    x =   heap[2*i + 1];
  }
}

static int find_unmarked_slot (void) {
  while (0 <= --hp) {
    if (!marks[hp]) return 1;
    marks[hp] = 0;
  }
  return 0;
}

static void collect_garbage (void) {
  int i;
  for (i = 0; i < sp; ++i)
    mark (stack[i]);
  hp = heap_size;
  if (!find_unmarked_slot ())
    die (\"Heap exhausted\");
}

static void push (obj x) {
  if (stack_size <= sp)
    die (\"Stack exhausted\");
  stack[sp++] = x;
}

static obj pop (void) {
  assert (0 < sp);
  return stack[--sp]; 
}

#define TOP                   ( stack[sp-1] )

#define typecheck(type, x)    ( assert ((type) == tag (x)) )

#define DEF_PRIM(name)        static void name (void)

DEF_PRIM(prim0_abort)         { die (\"Aborted\"); }

DEF_PRIM(prim1_nullP)         { TOP = boolean (TOP == nil); }
DEF_PRIM(prim1_pairP)         { TOP = boolean (tag (TOP) == a_pair); }
DEF_PRIM(prim1_symbolP)       { TOP = boolean (tag (TOP) == a_symbol); }
DEF_PRIM(prim1_stringP)       { TOP = boolean (tag (TOP) == a_string); }
DEF_PRIM(prim1_booleanP)      { TOP = boolean (tag (TOP) == a_boolean); }
DEF_PRIM(prim1_charP)         { TOP = boolean (tag (TOP) == a_char); }
DEF_PRIM(prim1_eof_objectP)   { TOP = boolean (TOP == eof); }

DEF_PRIM(prim1_car) { 
  typecheck (a_pair, TOP); check_ptr (bits (TOP));
  TOP = heap[2 * bits (TOP) + 0]; 
}
DEF_PRIM(prim1_cdr) { 
  typecheck (a_pair, TOP); check_ptr (bits (TOP));
  TOP = heap[2 * bits (TOP) + 1]; 
}

DEF_PRIM(prim2_cons) {
  if (!find_unmarked_slot ())
    collect_garbage ();
  heap[2*hp + 1] = pop();
  heap[2*hp + 0] = TOP;
  TOP = mkptr (a_pair, hp);
}

DEF_PRIM(prim1_write_char) { 
  typecheck (a_char, TOP);
  putc (bits (TOP), stdout);
  fflush (stdout);
}

DEF_PRIM(prim0_read_char) {
  int c = getc (stdin);
  push (EOF == c ? eof : mkchar (c));
}

DEF_PRIM(prim0_peek_char) {
  int c = getc (stdin);
  ungetc (c, stdin);
  push (EOF == c ? eof : mkchar (c));
}

DEF_PRIM(prim2_EEP) {
  obj y = pop ();
  TOP = boolean (TOP == y);
}

#define DEF_CVT(name, from_tag, to_tag) \\
  DEF_PRIM(name) {                      \\
    typecheck (from_tag, TOP);          \\
    TOP = mkptr (to_tag, bits (TOP));   \\
  }

DEF_CVT(prim1_explode, a_symbol, a_pair)
DEF_CVT(prim1_implode, a_pair, a_symbol)
DEF_CVT(prim1_string_Glist, a_string, a_pair)
DEF_CVT(prim1_list_Gstring, a_pair, a_string)

#define push_local(i)         ( push (stack[bp + (i)]) )

#define call(label, arity)    ( run ((label), sp - (arity)) )

#define tailcall(label, arity) do {        \\
    memmove (stack + bp, stack + sp - (arity), (arity) * sizeof stack[0]); \\
    sp = bp + (arity);                     \\
    pc = (label);                          \\
    goto again;                            \\
  } while (0)

#define answer()      do { \\
    stack[bp] = TOP;       \\
    sp = bp + 1;           \\
    return;                \\
  } while (0)
")
  (put-enum 'proc_ procedures)
  (put "
static void run (int pc, int bp) {
  again: switch (pc) {
"))

(define (emit-postlude)
  (put "
    default: die (\"Bug: unknown pc\");
  }
}
int main (void) {
  run (proc_main, 0);
  return 0;
}
"))

(define (emit-def-proc name params)
  (put #\newline)
  (put6 "case proc_" (map-c-id name) ":" #\newline "{" #\newline)
  (emit-params params))

(define (emit-end-proc)
  (put4 "answer();" #\newline "}" #\newline))

(define (emit-params params)
  (cond ((null? params))
        (#t (put-enum 'arg_ params)))
  (put4 "assert(sp-bp==" (my-length params) ");" #\newline))

(define (put-enum prefix ids)
  (put "enum { ")
  (put-id-list prefix ids)
  (put2 "};" #\newline))

(define (put-id-list prefix ids)
  (cond ((null? ids))
        (#t (put4 prefix (map-c-id (car ids)) #\, #\space)
            (put-id-list prefix (cdr ids)))))

(define (emit-if)    (put2 "if (false != pop ()) {" #\newline))
(define (emit-else)  (put2 "} else {" #\newline))
(define (emit-endif) (put2 "}" #\newline))
  
(define (emit-prim prim arity)
  (put6 'prim arity '_ (map-c-id prim) "();" #\newline))

(define (emit-call type id arity)
  (put7 type "(proc_" (map-c-id id) "," arity ");" #\newline))

(define (emit-local id)
  (put4 "push_local(arg_" (map-c-id id) ");" #\newline))

(define (emit-pop)
  (put2 "pop();" #\newline))

(define (emit-literal literal)
  (put "push(")
  (put-literal literal)
  (put2 ");" #\newline))

(define (put-literal lit)
  (cond ((==? lit '())     (put 'nil))
        ((==? lit #f)      (put 'false))
        ((==? lit #t)      (put 'true))
        ((char? lit)       (put "mkchar('")
                           (put-c-char lit)
                           (put "')"))
        ((eof-object? lit) (put 'eof))
        (#t                (put lit))))

(define (put-c-char c)
  (cond ((==? c #\tab)     (put2 #\\ #\t))
        ((==? c #\return)  (put2 #\\ #\r))
        ((==? c #\newline) (put2 #\\ #\n))
        ((==? c #\\)       (put2 #\\ #\\))
        ((==? c #\')       (put2 #\\ #\'))
        (#t                (put c))))

(define (map-c-id symbol)
  (implode (map-c-id-chars (explode symbol))))

(define (map-c-id-chars cs)
  (cond ((null? cs) '())
        (#t (cons (map-c-id-char (car cs))
                  (map-c-id-chars (cdr cs))))))

(define (map-c-id-char c)
  (cond ((==? c #\-) #\_)
        ((==? c #\=) #\E)
        ((==? c #\.) #\D)
        ((==? c #\>) #\G)
        ((==? c #\?) #\P)
        ((==? c #\_) #\U)
        (#t          c)))

(define (put z)
  (cond ((char? z) (write-char z))
        ((string? z) (write (implode (string->list z))))
        (#t (write z))))

(define (put2 y z)           (put y) (put z))
(define (put3 x y z)         (put x) (put2 y z))
(define (put4 w x y z)       (put w) (put3 x y z))
(define (put5 v w x y z)     (put v) (put4 w x y z))
(define (put6 u v w x y z)   (put u) (put5 v w x y z))
(define (put7 t u v w x y z) (put t) (put6 u v w x y z))

(define (my-length ls)
  (walk ls '(#\0 #\1 #\2 #\3 #\4 #\5 #\6 #\7 #\8 #\9)))

(define (walk xs ys)
  (cond ((null? xs) (car ys))
        (#t (walk (cdr xs) (cdr ys)))))
