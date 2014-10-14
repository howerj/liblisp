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
 *  Possible extras are:
 *
 *  @todo Add in syntax for quotes:
 *        '(list ...) become (quote (list ...))
 *        But make it optional
 *  @todo Add in syntax to get a line from a file like in perl
 *
 **/

#include "sexpr.h"
#include <string.h>
#include <ctype.h>
#include <assert.h>
#include "mem.h" 
#include "gc.h"  
#include "color.h"

static const char *octal_s = "01234567";
static const char *decimal_s = "0123456789";
static const char *hexadecimal_s = "0123456789abcdefABCDEF";

static bool color_on_f = false; /*turn color on/off */
static bool print_proc_f = false;       /*print actual code after #proc */
static bool parse_numbers_f = true;     /*parse numbers as numbers not symbols */

static bool indent(unsigned int depth, io * o);
static bool isnumber(const char *buf, size_t string_l);
static expr parse_symbol(io * i);  /* and integers (optionally) */
static expr parse_string(io * i);
static expr parse_list(io * i);
static bool parse_comment(io * i);

#define color_on(X,O) if(true==color_on_f){io_puts((X),(O));}

/*** interface functions *****************************************************/

/**
 *  @brief          Set whether to print out colors, *does not check if
 *                  we are printing to a terminal or not, it is either on
 *                  or off*.
 *  @param          flag boolean flag to set color_on_f
 *  @return         void
 **/
void sexpr_set_color_on(bool flag)
{
        color_on_f = flag;
}

/**
 *  @brief          Set whether we should print "<PROC>" when printing
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
        while (EOF != (c = io_getc(i))) {
                if (isspace(c)) {
                        continue;
                }
                switch (c) {
                case ')':
                        REPORT("unmatched ')'");
                        continue;
                case '#':
                        if (true == parse_comment(i))
                                return NULL;
                        continue;
                case '(':
                        return parse_list(i);
                case '"':
                        return parse_string(i);
                /*case '/': return parse_regex(i,e);*/
                /*case '<': return parse_file(i,e);*/
                /*case '\'': return parse_quote(i,e);*/
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
void sexpr_print(expr x, io * o, unsigned int depth)
{
        size_t i;

        if (NULL == x)
                return;

        switch (x->type) {
        case S_NIL:
                color_on(ANSI_COLOR_RED, o);
                io_puts("()", o);
                break;
        case S_TEE:
                color_on(ANSI_COLOR_GREEN, o);
                io_puts("t", o);
                break;
        case S_LIST:
                io_puts("(", o);
                for (i = 0; i < x->len; i++) {
                        if (0 != i) {
                                if (2 == x->len)
                                        io_putc(' ', o);
                                else
                                        indent(depth ? depth + 1 : 1, o);
                        }
                        sexpr_print(x->data.list[i], o, depth + 1);
                        if ((i < x->len - 1) && (2 != x->len))
                                io_putc('\n', o);
                }
                io_puts(")", o);
                break;
        case S_SYMBOL:         /*symbols are yellow, strings are red, escaped chars magenta */
        case S_STRING:
                {
                        bool isstring = S_STRING == x->type ? true : false;     /*isnotsymbol */
                        color_on(true == isstring ? ANSI_COLOR_RED : ANSI_COLOR_YELLOW, o);
                        if (isstring) {
                                io_putc('"', o);
                        }
                        for (i = 0; i < x->len; i++) {
                                switch ((x->data.string)[i]) {
                                case '"':
                                case '\\':
                                        color_on(ANSI_COLOR_MAGENTA, o);
                                        io_putc('\\', o);
                                        break;
                                case ')':
                                case '(':
                                case '#':
                                        if (x->type == S_SYMBOL) {
                                                color_on(ANSI_COLOR_MAGENTA, o);
                                                io_putc('\\', o);
                                        }
                                }
                                io_putc((x->data.string)[i], o);
                                color_on(true == isstring ? ANSI_COLOR_RED : ANSI_COLOR_YELLOW, o);
                        }
                        if (isstring)
                                io_putc('"', o);
                }
                break;
        case S_INTEGER:
                color_on(ANSI_COLOR_MAGENTA, o);
                io_printd(x->data.integer, o);
                break;
        case S_PRIMITIVE:
                color_on(ANSI_COLOR_BLUE, o);
                io_puts("<PRIMOP>", o);
                break;
        case S_PROC:
                if (true == print_proc_f) {
                        io_putc('\n', o);
                        indent(depth, o);
                        io_puts("(", o);
                        color_on(ANSI_COLOR_YELLOW, o);
                        io_puts("lambda\n", o);
                        color_on(ANSI_RESET, o);
                        indent(depth + 1, o);
                        sexpr_print(x->data.list[0], o, depth + 1);
                        io_putc('\n', o);
                        indent(depth + 1, o);
                        sexpr_print(x->data.list[1], o, depth + 1);
                        io_puts(")", o);
                } else {
                        color_on(ANSI_COLOR_BLUE, o);
                        io_puts("<PROC>", o);
                        color_on(ANSI_RESET, o);
                }
                break;
        case S_ERROR:
                /** @todo implement error support **/
        case S_FILE:
               /** @todo implement file support, then printing**/
                color_on(ANSI_COLOR_RED, o);
                REPORT("File/Error printing not supported!");
                break;
        case S_QUOTE: /*unimplemented, fallthrough...*/
        case S_LAST_TYPE: /*unimplemented, fallthrough...*/
        default:               /* should never get here */
                color_on(ANSI_COLOR_RED, o);
                REPORT("print: not a known printable type");
                color_on(ANSI_RESET, o);
                exit(EXIT_FAILURE);
                break;
        }
        color_on(ANSI_RESET, o);
        if (0 == depth)
                io_putc('\n', o);
        return;
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
#ifdef XXX
        if((NULL == e) && (NULL == fallback)){
                fallback = mem_calloc(1, io_sizeof_io(), NULL);
                io_file_out(fallback, stderr);
                e = fallback;
        }

        color_on(ANSI_BOLD_TXT, e);
        io_puts("(error \n \"", e);
        io_puts(msg, e);
        io_puts("\"\n \"", e);
        io_puts(cfile, e);
        io_puts("\"\n ", e);
        io_printd(linenum, e);
        if (NULL == x) {
        } else {
                io_puts("\n ", e);
                color_on(ANSI_RESET, e);
                sexpr_print(x, e, 1);
        }
        color_on(ANSI_BOLD_TXT, e);
        io_puts(")\n", e);
        color_on(ANSI_RESET, e);

        if(fallback == e){
                free(fallback);
                fallback = NULL;
        }
#endif
        return;
}

/**
 *  @brief          Takes an already existing list and appends an atom to it
 *  @param          list a list to append an atom to
 *  @param          ele  the atom to append to the list
 *  @return         void
 *
 *  @todo           Error handline
 *  @todo           Check for list type OR proc type
 **/
void append(expr list, expr ele)
{
        assert((NULL != list) && (NULL != ele));
        list->data.list = mem_realloc(list->data.list, sizeof(expr) * ++list->len);
        (list->data.list)[list->len - 1] = ele;
}

/*****************************************************************************/

/**
 *  @brief          Indent a line with spaces 'depth' amount of times
 *  @param          depth depth to indent to
 *  @param          o output stream
 *  @return         true on error, false on no error
 **/
static bool indent(unsigned int depth, io * o)
{
        unsigned int i;
        for (i = 0; i < depth; i++)
                if (EOF == io_putc(' ', o))
                        return true;
        return false;
}

/**
 *  @brief          Test whether buf is a number or not
 *  @param          buf       string to test for
 *  @param          string_l  length of string, avoids recalculation
 *  @return         true if it is a number, false otherwise
 **/

static bool isnumber(const char *buf, size_t string_l)
{
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
 *  @brief          parse a symbol or integer (in decimal or
 *                  octal format, positive or negative)
 *  @param          i input stream
 *  @return         NULL or parsed symbol / integer
 **/
static expr parse_symbol(io * i)
{                               /* and integers! */
        expr ex = NULL;
        unsigned int count = 0;
        int c;
        char buf[BUFLEN];
        ex = gc_calloc();

        memset(buf, '\0', BUFLEN);

        while (EOF != (c = io_getc(i))) {
                if (BUFLEN <= count) {
                        REPORT("symbol too long");
                        REPORT(buf);
                        goto fail;
                }

                if (isspace(c))
                        goto success;

                if ((c == '(') || (c == ')')) {
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
                        case '\\':
                        case '"':
                        case '(':
                        case ')':
                        case '#':
                                buf[count++] = c;
                                continue;
                        default:
                                REPORT(buf);
                                goto fail;
                        }
                case '"':
                        REPORT(buf);
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
        char buf[BUFLEN];

        ex = gc_calloc();
        memset(buf, '\0', BUFLEN);

        while (EOF != (c = io_getc(i))) {
                if (BUFLEN <= count) {
                        REPORT("string too long");
                        REPORT(buf); /* check if correct */
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
                                REPORT("invalid escape char");
                                REPORT(buf);
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
 *                  integers, or other lists.
 *  @param          i input stream
 *  @return         NULL or parsed list
 **/
static expr parse_list(io * i)
{
        expr ex = NULL, chld;
        int c;

        ex = gc_calloc();
        ex->len = 0;

        while (EOF != (c = io_getc(i))) {
                if (isspace(c))
                        continue;

                switch (c) {
                case '#':
                        if (true == parse_comment(i))
                                goto fail;
                        break;
                case '"':
                        chld = parse_string(i);
                        if (!chld)
                                goto fail;
                        append(ex, chld);
                        continue;
                case '(':
                        chld = parse_list(i);
                        if (!chld)
                                goto fail;
                        append(ex, chld);
                        continue;
                case ')':
                        goto success;

                default:
                        io_ungetc(c, i);
                        chld = parse_symbol(i);
                        if (!chld)
                                goto fail;
                        append(ex, chld);
                        continue;
                }
        }

 fail:
        REPORT("list err");
        return NULL;

 success:
        ex->type = S_LIST;
        return ex;
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
        while (EOF != (c = io_getc(i))) {
                if ('\n' == c) {
                        return false;
                }
        }
        return true;
}
