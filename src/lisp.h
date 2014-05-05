#ifndef LISP_H
#define LISP_H

typedef struct{ /** a lisp environment */
  io i;                   /** input */
  io o;                   /** output */
  io e;                   /** stderr */
  expr current;           /** current s-expr */
  expr global;            /** global list of key-value pairs
                            * (key val key val ... )
                            * Used to store things
                            * TODO: Store all values in an *ordered* list
                            */
} lispenv_t, *lisp;

#endif
