/**
 *  @file           sexpr.c
 *  @brief          General S-Expression parser
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v2.0 or later version
 *  @email          howe.r.j.89@gmail.com
 *  @details
 *
 *  This S-expression parser, although made for a lisp interpreter,
 *  is a small, generic, parser that could be used for other projects.
 *
 *  The three main functions are:
 *    - Parse an S-expression (sexpr_parse)
 *    - Print out an S-expression (sexpr_print)
 *    - Print out an S-expression (sexpr_perror) with possibly
 *      invalid arguments, this should be used for error messages.
 *
 *  There are sub-functions called by the parser that could be useful
 *  in their own right and might change so they can be accessed externally
 *  later.
 *
 *  A possible addition would be the ability to set what character
 *  demarks comments.
 *
 *  @todo Add in syntax for quotes:
 *        '(list ...) become (quote (list ...))
 *        But make it optional
 *  @todo Add in syntax for regexes
 *        (/regex/ list ...)
 *        (/find/substitute/ list ...)
 *        But make it optional
 *  @todo Add in syntax to get a line from a file
 *        <file>
 *  @todo Add in line numbers
 *  @todo Add in comments
 *  @todo Add syntax for executing shell commands directly
 *        (`command $arg_1 $arg_2 ...`)
 *
 **/

#include "type.h"               /* Project wide types */
#include "io.h"                 /* I/O wrappers */
#include "mem.h"                /* free wrappers */
#include "color.h"              /* ANSI color escape sequences */
#include <string.h>             /* strtol(), strspn(), strlen(), memset() */
#include <ctype.h>              /* isspace() */

static const char *octal_s = "01234567";
static const char *decimal_s = "0123456789";
static const char *hexadecimal_s = "0123456789abcdefABCDEF";

static bool color_on_f = false; /*turn color on/off */
static bool print_proc_f = false;       /*print actual code after #proc */
static bool parse_numbers_f = true;     /*parse numbers as numbers not symbols */

static bool indent(unsigned int depth, io * o, io * e);
static bool isnumber(const char *buf, size_t string_l);
static expr parse_symbol(io * i, io * e);       /* and integers (optionally) */
static expr parse_string(io * i, io * e);
static expr parse_list(io * i, io * e);
static bool parse_comment(io * i, io * e);

#define color_on(X,O,E) if(true==color_on_f){wputs((X),(O),(E));}

/*** interface functions *****************************************************/

/**
 *  @brief          Set whether to print out colors, *does not check if
 *                  we are printing to a terminal or not, it is either on
 *                  or off*.
 *  @param          flag boolean flag to set color_on_f
 *  @return         void
 **/
void set_color_on(bool flag)
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
void set_print_proc(bool flag)
{
        print_proc_f = flag;
}

/**
 *  @brief          If set to true (as is default) we parse numbers
 *                  as numbers and not as symbols
 *  @param          flag boolean flag to set parse_numbers_f
 *  @return         void
 **/
void set_parse_numbers(bool flag)
{
        parse_numbers_f = flag;
}

/**
 *  @brief          Parses a list or atom, returning the root
 *                  of the S-expression.
 *  @param          i input stream
 *  @param          e error output stream
 *  @return         NULL or parsed list
 **/
expr sexpr_parse(io * i, io * e)
{
        int c;
        while (EOF != (c = wgetc(i, e))) {
                if (isspace(c)) {
                        continue;
                }
                switch (c) {
                case ')':
                        report("unmatched ')'", e);
                        continue;
                case '#':
                        if (true == parse_comment(i, e))
                                return NULL;
                        continue;
                case '(':
                        return parse_list(i, e);
                case '"':
                        return parse_string(i, e);
                /*case '/': return parse_regex(i,e);*/
                /*case '<': return parse_file(i,e);*/
                /*case '\'': return parse_quote(i,e);*/
                default:
                        wungetc(c, i, e);
                        return parse_symbol(i, e);
                }
        }
        return NULL;
}

/**
 *  @brief          Recursively prints out an s-expression
 *  @param          x     expression to print
 *  @param          o     output stream
 *  @param          depth current depth of expression
 *  @param          e     error output stream
 *  @return         void
 **/
void sexpr_print(expr x, io * o, unsigned int depth, io * e)
{
        unsigned int i;

        if (NULL == x)
                return;

        switch (x->type) {
        case S_NIL:
                color_on(ANSI_COLOR_RED, o, e);
                wputs("()", o, e);
                break;
        case S_TEE:
                color_on(ANSI_COLOR_GREEN, o, e);
                wputs("t", o, e);
                break;
        case S_LIST:
                wputs("(", o, e);
                for (i = 0; i < x->len; i++) {
                        if (0 != i) {
                                if (2 == x->len)
                                        wputc(' ', o, e);
                                else
                                        indent(depth ? depth + 1 : 1, o, e);
                        }
                        sexpr_print(x->data.list[i], o, depth + 1, e);
                        if ((i < x->len - 1) && (2 != x->len))
                                wputc('\n', o, e);
                }
                wputs(")", o, e);
                break;
        case S_SYMBOL:         /*symbols are yellow, strings are red, escaped chars magenta */
        case S_STRING:
                {
                        bool isstring = S_STRING == x->type ? true : false;     /*isnotsymbol */
                        color_on(true == isstring ? ANSI_COLOR_RED : ANSI_COLOR_YELLOW, o, e);
                        if (isstring) {
                                wputc('"', o, e);
                        }
                        for (i = 0; i < x->len; i++) {
                                switch ((x->data.string)[i]) {
                                case '"':
                                case '\\':
                                        color_on(ANSI_COLOR_MAGENTA, o, e);
                                        wputc('\\', o, e);
                                        break;
                                case ')':
                                case '(':
                                case '#':
                                        if (x->type == S_SYMBOL) {
                                                color_on(ANSI_COLOR_MAGENTA, o, e);
                                                wputc('\\', o, e);
                                        }
                                }
                                wputc((x->data.string)[i], o, e);
                                color_on(true == isstring ? ANSI_COLOR_RED : ANSI_COLOR_YELLOW, o, e);
                        }
                        if (isstring)
                                wputc('"', o, e);
                }
                break;
        case S_INTEGER:
                color_on(ANSI_COLOR_MAGENTA, o, e);
                wprintd(x->data.integer, o, e);
                break;
        case S_PRIMITIVE:
                color_on(ANSI_COLOR_BLUE, o, e);
                wputs("<PRIMOP>", o, e);
                break;
        case S_PROC:
                if (true == print_proc_f) {
                        wputc('\n', o, e);
                        indent(depth, o, e);
                        wputs("(", o, e);
                        color_on(ANSI_COLOR_YELLOW, o, e);
                        wputs("lambda\n", o, e);
                        color_on(ANSI_RESET, o, e);
                        indent(depth + 1, o, e);
                        sexpr_print(x->data.list[0], o, depth + 1, e);
                        wputc('\n', o, e);
                        indent(depth + 1, o, e);
                        sexpr_print(x->data.list[1], o, depth + 1, e);
                        wputs(")", o, e);
                } else {
                        color_on(ANSI_COLOR_BLUE, o, e);
                        wputs("<PROC>", o, e);
                        color_on(ANSI_RESET, o, e);
                }
                break;
        case S_ERROR:
                /** @todo implement error support **/
        case S_FILE:
               /** @todo implement file support, then printing**/
                color_on(ANSI_COLOR_RED, o, e);
                report("File/Error printing not supported!", e);
                break;
        default:               /* should never get here */
                color_on(ANSI_COLOR_RED, o, e);
                report("print: not a known printable type", e);
                color_on(ANSI_RESET, o, e);
                exit(EXIT_FAILURE);
                break;
        }
        color_on(ANSI_RESET, o, e);
        if (0 == depth)
                wputc('\n', o, e);
        return;
}

/**
 *  @brief          Print out an error message while trying to handle
 *                  invalid arguments gracefully. A macro called
 *                  sexpr_perror should be defined in the header so you
 *                  do not have to pass cfile and linenum constantly to it.
 *  @param          x       NULL or expression to print
 *  @param          msg     Error message to print
 *  @param          cfile   File error occurred in 
 *  @param          linenum Line number error occurred on
 *  @param          e       Output wrapper stream, if NULL, default to stderr
 *  @return         void
 **/
void dosexpr_perror(expr x, char *msg, char *cfile, unsigned int linenum, io * e)
{
        static io fallback = { file_out, {NULL}, 0, 0, 0, false };
        fallback.ptr.file = stderr;

        if ((NULL == e) || (NULL == e->ptr.file))
                e = &fallback;

        color_on(ANSI_BOLD_TXT, e, e);
        wputs("(error \n \"", e, e);
        wputs(msg, e, e);
        wputs("\"\n \"", e, e);
        wputs(cfile, e, e);
        wputs("\"\n ", e, e);
        wprintd(linenum, e, e);
        if (NULL == x) {
        } else {
                wputs("\n ", e, e);
                color_on(ANSI_RESET, e, e);
                sexpr_print(x, e, 1, e);
        }
        color_on(ANSI_BOLD_TXT, e, e);
        wputs(")\n", e, e);
        color_on(ANSI_RESET, e, e);
        return;
}

/**
 *  @brief          Takes an already existing list and appends an atom to it
 *  @param          list a list to append an atom to
 *  @param          ele  the atom to append to the list
 *  @param          e    error output stream
 *  @return         void
 *
 *  @todo           Error handline
 *  @todo           Check for list type OR proc type
 **/
void append(expr list, expr ele, io * e)
{
        NULLCHK(list, e);
        NULLCHK(ele, e);
        list->data.list = wrealloc(list->data.list, sizeof(expr) * ++list->len, e);
        (list->data.list)[list->len - 1] = ele;
}

/*****************************************************************************/

/**
 *  @brief          Indent a line with spaces 'depth' amount of times
 *  @param          depth depth to indent to
 *  @param          o output stream
 *  @param          e error output stream
 *  @return         true on error, false on no error
 **/
static bool indent(unsigned int depth, io * o, io * e)
{
        unsigned int i;
        for (i = 0; i < depth; i++)
                if (EOF == wputc(' ', o, e))
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
 *  @param          e error output stream
 *  @return         NULL or parsed symbol / integer
 **/
static expr parse_symbol(io * i, io * e)
{                               /* and integers! */
        expr ex = NULL;
        unsigned int count = 0;
        int c;
        char buf[BUFLEN];
        ex = gccalloc(e);

        memset(buf, '\0', BUFLEN);

        while (EOF != (c = wgetc(i, e))) {
                if (BUFLEN <= count) {
                        report("symbol too long", e);
                        report(buf, e);
                        goto fail;
                }

                if (isspace(c))
                        goto success;

                if ((c == '(') || (c == ')')) {
                        wungetc(c, i, e);
                        goto success;
                }

                if (c == '#') {
                        parse_comment(i, e);
                        goto success;
                }

                switch (c) {
                case '\\':
                        switch (c = wgetc(i, e)) {
                        case '\\':
                        case '"':
                        case '(':
                        case ')':
                        case '#':
                                buf[count++] = c;
                                continue;
                        default:
                                report(buf, e);
                                goto fail;
                        }
                case '"':
                        report(buf, e);
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
                ex->data.integer = strtol(buf, NULL, 0);
        } else {
                ex->type = S_SYMBOL;
                ex->data.symbol = wmalloc(ex->len + 1, e);
                strcpy(ex->data.symbol, buf);
        }
        return ex;
}

/**
 *  @brief          parse a string into a s-expr atom
 *  @param          i input stream
 *  @param          e error output stream
 *  @return         NULL or parsed string
 **/
static expr parse_string(io * i, io * e)
{
        expr ex = NULL;
        unsigned int count = 0;
        int c;
        char buf[BUFLEN];

        ex = gccalloc(e);
        memset(buf, '\0', BUFLEN);

        while (EOF != (c = wgetc(i, e))) {
                if (BUFLEN <= count) {
                        report("string too long", e);
                        report(buf, e); /* check if correct */
                        goto fail;
                }
                switch (c) {
                case '\\':
                        switch (c = wgetc(i, e)) {
                        case '\\':
                        case '"':
                                buf[count++] = c;
                                continue;
                        default:
                                report("invalid escape char", e);
                                report(buf, e);
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
        ex->data.string = wmalloc(ex->len + 1, e);
        strcpy(ex->data.string, buf);
        return ex;
}

/**
 *  @brief          Parses a list, consisting of strings, symbols,
 *                  integers, or other lists.
 *  @param          i input stream
 *  @param          e error output stream
 *  @return         NULL or parsed list
 **/
static expr parse_list(io * i, io * e)
{
        expr ex = NULL, chld;
        int c;

        ex = gccalloc(e);
        ex->len = 0;

        while (EOF != (c = wgetc(i, e))) {
                if (isspace(c))
                        continue;

                switch (c) {
                case '#':
                        if (true == parse_comment(i, e))
                                goto fail;
                        break;
                case '"':
                        chld = parse_string(i, e);
                        if (!chld)
                                goto fail;
                        append(ex, chld, e);
                        continue;
                case '(':
                        chld = parse_list(i, e);
                        if (!chld)
                                goto fail;
                        append(ex, chld, e);
                        continue;
                case ')':
                        goto success;

                default:
                        wungetc(c, i, e);
                        chld = parse_symbol(i, e);
                        if (!chld)
                                goto fail;
                        append(ex, chld, e);
                        continue;
                }
        }

 fail:
        report("list err", e);
        return NULL;

 success:
        ex->type = S_LIST;
        return ex;
}

/**
 *  @brief          Parses a comment, in the future, we might want to parse this
 *                  and return it as an expression so it can be used later.
 *  @param          i input stream
 *  @param          e error output stream
 *  @return         boolean; true on error/EOF, false on no error / no EOF
 **/
static bool parse_comment(io * i, io * e)
{
        int c;
        while (EOF != (c = wgetc(i, e))) {
                if ('\n' == c) {
                        return false;
                }
        }
        return true;
}
