#ifndef LISP_H
#define LISP_H

typedef struct{ /** a lisp environment */
  io i;                   /** input */
  io o;                   /** output */
  io e;                   /** stderr */
  expr current;           /** current s-expr */
  expr global;            /** global s-expr */
} lispenv_t, *lisp;

#endif
