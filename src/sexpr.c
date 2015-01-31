/**
 *  @file           sexpr.c
 *  @brief          General S-Expression parser
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        LGPL v2.1 or later version
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 *  This S-expression parser, although made for a lisp interpreter,
 *  is a small, generic, parser that could be used for other projects.
 *
 *  The three main functions are:
 *    - Parse an S-expression (sexpr_parse)
 *    - Print out an S-expression (sexpr_print)
 *    - Print out an S-expression (SEXPR_PERROR) with possibly
 *      invalid arguments, this should be used for error messages.
 *
 *  There are sub-functions called by the parser that could be useful
 *  in their own right and might change so they can be accessed externally
 *  later.
 *
 *  @todo The parser should be able to handle arbitrary length strings, symbols
 *        and numbers
 *  @todo Make sure arbitrary binary data is handled correctly, but that's
 *        throughout the code.
 *  @todo Add reference to the code this is based on
 *  @todo Needs to handle cons properly; (x . y) and (x . ()) = (x)
 *
 **/

#include "sexpr.h"
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "mem.h" 
#include "gc.h"  

static const char *octal_s = "01234567";
static const char *decimal_s = "0123456789";
static const char *hexadecimal_s = "0123456789abcdefABCDEF";

static bool print_proc_f = false;       /*print actual code after proc */
static bool parse_numbers_f = true;     /*parse numbers as numbers not symbols */

static bool isnumber(const char *buf, size_t string_l);
static expr parse_quote(io * i); 
static expr parse_symbol(io * i);  /* and integers (optionally) */
static expr parse_string(io * i);
static expr parse_list(io * i);
static bool parse_comment(io * i);

/*** interface functions *****************************************************/

/**
 *  @brief          Set whether we should print "{proc}" when printing
 *                  a user defined procedure, or if we should actually
 *                  print the contents of said procedure fully.
 *  @param          flag boolean flag to set print_proc_f
 *  @return         void
 **/
void sexpr_set_print_proc(bool flag)
{
        print_proc_f = flag;
}

/**
 *  @brief          If set to true (as is default) we parse numbers
 *                  as numbers and not as symbols
 *  @param          flag boolean flag to set parse_numbers_f
 *  @return         void
 **/
void sexpr_set_parse_numbers(bool flag)
{
        parse_numbers_f = flag;
}

/**
 *  @brief          Parses a list or atom, returning the root
 *                  of the S-expression.
 *  @param          i input stream
 *  @return         NULL or parsed list
 **/
expr sexpr_parse(io * i)
{
        int c;
        if(NULL == i)
                return NULL; /*@todo return error instead*/
        while (EOF != (c = io_getc(i))) {
                if (isspace(c))
                        continue;
                switch (c) {
                case ')':
                        IO_REPORT("unmatched ')'");
                        continue;
                case '#':
                        if (true == parse_comment(i))
                                return NULL;
                        continue;
                case '(':  return parse_list(i);
                case '"':  return parse_string(i);
                case '\'': return parse_quote(i);
                /*case '/': return parse_regex(i);*/
                /*case '<': return parse_file(i);*/
                default:
                        io_ungetc(c, i);
                        return parse_symbol(i);
                }
        }
        return NULL;
}

/**
 *  @brief          Recursively prints out an s-expression
 *  @param          x     expression to print
 *  @param          o     output stream
 *  @param          depth current depth of expression
 *  @return         void
 **/
void sexpr_print(expr x, io * o, unsigned depth)
{
        size_t i;
        io *e;
        if(NULL == x)
                return;
        switch(x->type){
        case S_NIL:       io_printer(o,"%r()"); break;
        case S_TEE:       io_printer(o,"%gt");  break;
        case S_INTEGER:   io_printer(o,"%m%d", x->data.integer); break;
        case S_PRIMITIVE: io_printer(o,"%b{prim}"); break;
        case S_PROC:      io_printer(o,"%b{proc}"); break;
        case S_FILE:      /*not implemented yet*/ break;
        case S_ERROR:     /*not implemented yet*/ break;
        case S_CONS:      /*does not handle (x . y) yet*/
                io_putc('(', o);
                do{
                        if(x->data.cons[0])
                                sexpr_print(x->data.cons[0],o,depth+1);
                        if(x->data.cons[1] && x->data.cons[1]->data.cons[1])
                                io_putc(' ',o);
                } while((NULL != x) && (NULL != (x = x->data.cons[1])));
                io_putc(')', o);
                break;
        case S_STRING: /*fall through*/
        case S_SYMBOL: /*symbols are yellow, strings are red, escaped chars magenta */
        {
                bool isstring = S_STRING == x->type ? true : false;     /*isnotsymbol */
                io_printer(o, isstring ? "%r" : "%y");
                if (isstring)
                        io_putc('"', o);
                for (i = 0; i < x->len; i++) {
                        switch ((x->data.string)[i]) {
                        case '"': case '\\':
                                io_printer(o, "%m\\");
                                break;
                        case ')': case '(': case '#':
                                if (x->type == S_SYMBOL) 
                                        io_printer(o, "%m\\");
                        default:
                                break;
                        }
                        io_putc((x->data.string)[i], o);
                        io_printer(o, isstring ? "%r" : "%y");
                }
                if (isstring)
                        io_putc('"', o);
        }
        break;
        case S_QUOTE: 
                io_putc('\'',o);
                sexpr_print(x->data.quoted,o,depth);
                break;
        case S_LAST_TYPE: /*fall through, not a type*/
        default:
                e = io_get_error_stream();
                io_printer(e,"%r");
                IO_REPORT("print: not a known printable type");
                io_printer(e,"%t");
                exit(EXIT_FAILURE);
                break;
        }
        io_printer(o,"%t");
        if (0 == depth)
                io_putc('\n', o);
}

/**
 *  @brief          Print out an error message while trying to handle
 *                  invalid arguments gracefully. A macro called
 *                  SEXPR_PERROR should be defined in the header so you
 *                  do not have to pass cfile and linenum constantly to it.
 *  @param          x       NULL or expression to print
 *  @param          msg     Error message to print
 *  @param          cfile   File error occurred in 
 *  @param          linenum Line number error occurred on
 *  @return         void
 **/
void dosexpr_perror(expr x, char *msg, char *cfile, unsigned int linenum)
{
        static io *fallback; 
        io *e = io_get_error_stream();
        if((NULL == e) && (NULL == fallback)){
                fallback = mem_calloc(1, io_sizeof_io());
                io_file_out(fallback, stderr);
                e = fallback;
        }

        io_printer(e, "%B(error\n \"%s\"\n \"%s\"\n %d",msg, cfile, linenum);
        if (NULL != x) {
                io_printer(e, "\n %t");
                sexpr_print(x, e, 1);
        }
        io_printer(e, "%B)%t\n");

        if(fallback == e){
                free(fallback);
                fallback = NULL;
        }
        return;
}

/**
 *  @brief          Takes an already existing cons cell and appends an atom to it
 *                      - New cons (nil, NULL)
 *                      - Old cons (ele, new cons)
 *                      - Return New cons
 *  @param          cons a list to append an atom to
 *  @param          ele  the atom to append to the list
 *  @return         New cons cell
 **/
expr append(expr cons, expr ele)
{
        expr nc = NULL;
        assert(cons && ele);
        nc = gc_calloc();
        cons->data.cons[0] = ele;
        cons->data.cons[1] = nc;
        nc->type = S_CONS;
        nc->data.cons[0] = nc->data.cons[1] = NULL;
        return nc;
}

/*****************************************************************************/

/**
 *  @brief          Test whether buf is a number or not
 *  @param          buf       string to test for
 *  @param          string_l  length of string, avoids recalculation
 *  @return         true if it is a number, false otherwise
 **/

static bool isnumber(const char *buf, size_t string_l)
{
        assert(buf);
        if ('-' == buf[0] || ('+' == buf[0])) {
                /*don't want negative hex/octal or + - symbols */
                if ((1 == string_l) || ('0' == buf[1]))
                        return 0;
                else
                        return strspn(buf + 1, decimal_s) == (string_l - 1);
        }
        if ('0' == buf[0] && (string_l > 1)) {
                if (('x' == buf[1]) || ('X' == buf[1]))
                        return strspn(buf + 2, hexadecimal_s) == (string_l - 2);
                else {
                        return strspn(buf, octal_s) == string_l;
                }

        }
        return strspn(buf, decimal_s) == string_l;
}

/**
 *  @brief          parse a quoted expressions
 *  @param          i     input stream
 *  @return         NULL or parsed quote
 **/
expr parse_quote(io * i){
        expr ex = NULL;
        assert(i);
        ex = gc_calloc();
        ex->type = S_QUOTE;
        if(NULL == (ex->data.quoted = sexpr_parse(i)))
                return NULL;
        return ex;
}

/**
 *  @brief          parse a symbol or integer (in decimal or
 *                  octal format, positive or negative)
 *  @param          i input stream
 *  @return         NULL or parsed symbol / integer
 **/
static expr parse_symbol(io * i)
{
        expr ex = NULL;
        unsigned count = 0;
        int c;
        char buf[SEXPR_BUFLEN];
        ex = gc_calloc();
        assert(i);

        memset(buf, '\0', SEXPR_BUFLEN);

        while (EOF != (c = io_getc(i))) {
                if (SEXPR_BUFLEN <= count) {
                        IO_REPORT("symbol too long");
                        IO_REPORT(buf);
                        goto fail;
                }

                if (isspace(c))
                        goto success;

                if ((c == '(') || (c == ')') || (c == '\'')) {
                        io_ungetc(c, i);
                        goto success;
                }

                if (c == '#') {
                        parse_comment(i);
                        goto success;
                }

                switch (c) {
                case '\\':
                        switch (c = io_getc(i)) {
                        /*should do other isspace chars as well*/
                        case '\\': case ' ': case '"':
                        case '(':  case ')': case '#':
                        case '\t': case '\'':
                                buf[count++] = c;
                                continue;
                        default:
                                IO_REPORT(buf);
                                goto fail;
                        }
                case '"':
                        IO_REPORT("You should separate symbols and strings!");
                        IO_REPORT(buf);
                        goto success;
                default:
                        buf[count++] = c;
                }
        }
 fail:
        return NULL;
 success:
        ex->len = strlen(buf);
        if ((true == parse_numbers_f) && isnumber(buf, ex->len)) {
                ex->type = S_INTEGER;
                ex->data.integer = (int32_t)strtol(buf, NULL, 0);
        } else {
                ex->type = S_SYMBOL;
                ex->data.symbol = mem_malloc(ex->len + 1);
                strcpy(ex->data.symbol, buf);
        }
        return ex;
}

/**
 *  @brief          parse a string into a s-expr atom
 *  @param          i input stream
 *  @return         NULL or parsed string
 **/
static expr parse_string(io * i)
{
        expr ex = NULL;
        unsigned int count = 0;
        int c;
        char buf[SEXPR_BUFLEN];
        assert(i);

        ex = gc_calloc();
        memset(buf, '\0', SEXPR_BUFLEN);

        while (EOF != (c = io_getc(i))) {
                if (SEXPR_BUFLEN <= count) {
                        IO_REPORT("string too long");
                        IO_REPORT(buf); /* check if correct */
                        goto fail;
                }
                switch (c) {
                case '\\':
                        switch (c = io_getc(i)) {
                        case '\\':
                        case '"':
                                buf[count++] = c;
                                continue;
                        default:
                                IO_REPORT("invalid escape char");
                                IO_REPORT(buf);
                                goto fail;
                        }
                case '"':
                        goto success;
                default:
                        buf[count++] = c;
                }
        }
 fail:
        return NULL;
 success:
        ex->type = S_STRING;
        ex->len = strlen(buf);
        ex->data.string = mem_malloc(ex->len + 1);
        strcpy(ex->data.string, buf);
        return ex;
}

/**
 *  @brief          Parses a list, consisting of strings, symbols,
 *                  integers, quotes, or other lists into a list of
 *                  cons cells
 *  @param          i input stream
 *  @return         NULL or parsed list
 **/
static expr parse_list(io * i)
{
        expr ex = NULL, head = NULL, prev = NULL, chld;
        int c;
        assert(i);

        head = ex = gc_calloc();
        head->type = S_CONS;

        while (EOF != (c = io_getc(i))) {
                if (isspace(c))
                        continue;

                prev = ex;
                switch (c) {
                case '#':
                        if (true == parse_comment(i))
                                goto fail;
                        break;
                case '"':
                        chld = parse_string(i);
                        if (!chld || !(ex = append(ex,chld)))
                                goto fail;
                        continue;
                case '\'':
                        chld = parse_quote(i);
                        if (!chld || !(ex = append(ex,chld)))
                                goto fail;
                        continue;
                case '(':
                        chld = parse_list(i);
                        if (!chld || !(ex = append(ex,chld)))
                                goto fail;
                        continue;
                case ')':
                        goto success;
                default:
                        io_ungetc(c, i);
                        chld = parse_symbol(i);
                        if (!chld || !(ex = append(ex,chld)))
                                goto fail;
                        continue;
                }
        }
 fail:
        IO_REPORT("list err");
        return NULL;
 success:
        if(ex == head)
                head->type = S_NIL;
        if(prev->data.cons[1] && !ex->data.cons[0])
                prev->data.cons[1] = NULL;
        return head;
}

/**
 *  @brief          Parses a comment, in the future, we might want to parse this
 *                  and return it as an expression so it can be used later.
 *  @param          i input stream
 *  @return         boolean; true on error/EOF, false on no error / no EOF
 **/
static bool parse_comment(io * i)
{
        int c;
        assert(i);
        while (EOF != (c = io_getc(i))) 
                if ('\n' == c) 
                        return false;
        return true;
}

