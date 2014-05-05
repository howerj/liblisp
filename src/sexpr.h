#ifndef SEXPR_H
#define SEXPR_H

expr parse_term(io *i, io *e);
void print_expr(expr x, io *o, unsigned int depth, io *e);
void free_expr(expr x, io *e);
void append(expr list, expr ele, io *e);

#endif
