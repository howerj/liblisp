/* 
 * Richard James Howe
 * Howe Lisp.
 *
 * Lisp interpreter.
 *
 * @author         Richard James Howe.
 * @copyright      Copyright 2013 Richard James Howe.
 * @license        LGPL      
 * @email          howe.r.j.89@gmail.com
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "lisp.h"

/*****************************************************************************/
static int wrap_get(file_io_t * in);
static int wrap_put(file_io_t * out, char c);
static void wrap_ungetc(file_io_t * in, char c);

static cell_t *parse_string(file_io_t * in, file_io_t * err);
static cell_t *parse_symbol(file_io_t * in, file_io_t * err);
/*static cell_t *parse_int(file_io_t *in);*/
static int append(cell_t * head, cell_t * child);
static cell_t *parse_list(file_io_t * in, file_io_t * err);
static void print_space(int depth, file_io_t * out);
static void print_string(char *s, file_io_t * out);
/* 
\\ "cell_t *list" will need replacing with a "lisp" object
\\ containing everything the interpreter needs to run.
int     lisp_interpret(cell_t *list);
int     lisp_monitor(cell_t *list);
*/

/*****************************************************************************/
#define error(X) do{\
    fprintf(stderr, "(error \"%s\" \"%s\" %d)\n",(X),__FILE__,__LINE__);\
  }while(0);

/*Input / output wrappers.*/

static int wrap_get(file_io_t * in)
{
        char tmp;
        if (in->ungetc_flag == true) {
                in->ungetc_flag = false;
                return in->ungetc_char;
        }

        switch (in->fiot) {
        case io_stdin:
                return getc(stdin);
        case io_rd_file:
                if (in->io_ptr.f != NULL)
                        return fgetc(in->io_ptr.f);
                else
                        return EOF;
        case io_rd_str:
                if (in->io_ptr.s != NULL) {
                        if (in->str_index > in->str_max_len)
                                return EOF;
                        tmp = in->io_ptr.s[++in->str_index];
                        if (tmp != '\0')
                                return (int)tmp;
                        else
                                return EOF;
                } else
                        return EOF;
        default:
                return EOF;
        }
}

static int wrap_put(file_io_t * out, char c)
{
        switch (out->fiot) {
        case io_stdout:
                return putc(c, stdout);
        case io_stderr:
                return putc(c, stderr);
        case io_wr_file:
                if (out->io_ptr.f != NULL)
                        return fputc(c, out->io_ptr.f);
                else
                        return EOF;
        case io_wr_str:
                if (out->io_ptr.s != NULL) {
                        if (out->str_index > out->str_max_len)
                                return EOF;
                        out->io_ptr.s[++(out->str_index)] = c;
                } else {
                        return EOF;
                }
                return ERR_OK;
        default:
                return EOF;
        }
}

static void wrap_ungetc(file_io_t * in, char c)
{
        in->ungetc_flag = true;
        in->ungetc_char = c;
        return;
}

/*****************************************************************************/

/*parse a string*/
static cell_t *parse_string(file_io_t * in, file_io_t * err)
{
        char buf[MAX_STR];
        int i = 0, c;
        cell_t *cell_str = calloc(1, sizeof(cell_t));
        if (cell_str == NULL) {
                error("calloc() failed");
                return NULL;
        }

        while ((c = wrap_get(in)) != EOF) {
                if (i >= MAX_STR) {
                        error("String too long.");
                        goto FAIL;
                }

                switch (c) {
                case '"':      /*success */
                        cell_str->type = type_str;
                        /*could add in string length here. */
                        cell_str->car.s = calloc(i + 1, sizeof(char));
                        if (cell_str->car.s == NULL) {
                                error("calloc() failed");
                                goto FAIL;
                        }
                        memcpy(cell_str->car.s, buf, i);
                        cell_str->cdr.cell = NULL;      /*stating this explicitly. */
                        return cell_str;
                case '\\':     /*escape char */
                        switch (c = wrap_get(in)) {
                        case '\\':     /*fall through */
                        case '"':
                                buf[i] = c;
                                i++;
                                continue;
                        case 'n':
                                buf[i] = '\n';
                                i++;
                                continue;
                        case EOF:
                                error
                                    ("EOF encountered while processing escape char");
                                goto FAIL;
                        default:
                                error("Not an escape character");
                                goto FAIL;
                        }
                case EOF:
                        error("EOF encountered while processing symbol");
                        goto FAIL;
                default:       /*just add the character into the buffer */
                        buf[i] = c;
                        i++;
                }
        }
 FAIL:
        error("parsing string failed.");
        free(cell_str);
        return NULL;
}

/*parse a symbol*/
static cell_t *parse_symbol(file_io_t * in, file_io_t * err)
{
        char buf[MAX_STR];
        int i = 0, c;
        cell_t *cell_sym = calloc(1, sizeof(cell_t));
        if (cell_sym == NULL) {
                error("calloc() failed");
                return NULL;
        }

        while ((c = wrap_get(in)) != EOF) {
                if (i >= MAX_STR) {
                        error("String (symbol) too long.");
                        goto FAIL;
                }
                if (isspace(c))
                        goto SUCCESS;

                if (c == '(' || c == ')') {
                        wrap_ungetc(in, c);
                        goto SUCCESS;
                }

                switch (c) {
                case '\\':
                        switch (c = wrap_get(in)) {
                        case '"':
                        case '(':
                        case ')':
                                buf[i] = c;
                                i++;
                                continue;
                        default:
                                error("Not an escape character");
                                goto FAIL;
                        }
                case '"':
                        error("Unescaped \" or incorrectly formatted input.");
                        goto FAIL;
                case EOF:
                        error("EOF encountered while processing symbol");
                        goto FAIL;
                default:       /*just add the character into the buffer */
                        buf[i] = c;
                        i++;
                }
        }
 SUCCESS:
        cell_sym->car.s = calloc(i + 1, sizeof(char));
        if (cell_sym->car.s == NULL) {
                error("calloc() failed");
                goto FAIL;
        }
        cell_sym->type = type_symbol;
        memcpy(cell_sym->car.s, buf, i);
        cell_sym->cdr.cell = NULL;      /*stating this explicitly. */
        return cell_sym;
 FAIL:
        error("parsing symbol failed.");
        free(cell_sym);
        return NULL;
}

static int append(cell_t * head, cell_t * child)
{
        if (head == NULL || child == NULL) {
                error("append failed, head or child is null");
                return 1;
        }
        head->cdr.cell = child;
        return 0;
}

static cell_t *parse_list(file_io_t * in, file_io_t * err)
{
        cell_t *head, *child = NULL, *cell_lst = calloc(1, sizeof(cell_t));
        int c;
        head = cell_lst;
        if (cell_lst == NULL) {
                error("calloc() failed");
                return NULL;
        }
        while ((c = wrap_get(in)) != EOF) {
                if (isspace(c))
                        continue;

                switch (c) {
                case ')':      /*finished */
                        goto SUCCESS;
                case '(':      /*encountered a nested list */
                        child = calloc(1, sizeof(cell_t));
                        if (child == NULL)
                                goto FAIL;
                        head->cdr.cell = child;
                        child->car.cell = parse_list(in, err);
                        if (child->car.cell == NULL)
                                goto FAIL;
                        child->type = type_list;
                        head = child;
                        break;
                case '"':      /*parse string */
                        child = parse_string(in, err);
                        if (append(head, child))
                                goto FAIL;
                        head = child;
                        break;
                case EOF:      /*Failed */
                        error("EOF occured before end of list did.");
                        goto FAIL;
                default:       /*parse symbol */
                        wrap_ungetc(in, c);
                        child = parse_symbol(in, err);
                        if (append(head, child))
                                goto FAIL;
                        head = child;
                        break;
                }

        }
 SUCCESS:
        cell_lst->type = type_list;
        return cell_lst;
 FAIL:
        error("parsing list failed.");
        free(cell_lst);
        return NULL;
}

cell_t *parse_sexpr(file_io_t * in, file_io_t * err)
{
        int c;
        if (in != NULL)
                while ((c = wrap_get(in)) != '\0') {
                        if (isspace(c))
                                continue;

                        switch (c) {
                        case '(':
                                return parse_list(in, err);
                        case '"':
                                return parse_string(in, err);
                        case EOF:
                                error("EOF, nothing to parse");
                                return NULL;
                        case ')':
                                error("Unmatched ')'");
                                return NULL;
                        default:
                                wrap_ungetc(in, c);
                                return parse_symbol(in, err);
                        }

                }

        error("parse_expr in == NULL");
        return NULL;
}

static void print_space(int depth, file_io_t * out)
{
        int i;
        for (i = 0; i < ((depth * 2)); i++)
                (void)wrap_put(out, ' ');
}

static void print_string(char *s, file_io_t * out)
{
        int i;
        for (i = 0; s[i] != '\0'; i++)
                (void)wrap_put(out, s[i]);
}

void print_sexpr(cell_t * list, int depth, file_io_t * out, file_io_t * err)
{
        cell_t *tmp;
        if (list == NULL) {
                error("print_sexpr was passed a NULL!");
                return;
        }

        if (list->type == type_null) {
                print_space(depth + 1, out);
                print_string("Null\n", out);
                return;
        } else if (list->type == type_str) {
                print_space(depth + 1, out);
                wrap_put(out, '"');
                print_string(list->car.s, out);
                wrap_put(out, '"');
                wrap_put(out, '\n');
                /*printf("\"%s\"\n", list->car.s); */
                return;
        } else if (list->type == type_symbol) {
                print_space(depth + 1, out);
                print_string(list->car.s, out);
                wrap_put(out, '\n');
                return;
        } else if (list->type == type_list) {
                if (depth == 0) {
                        print_string("(\n", out);
                }
                for (tmp = list; tmp != NULL; tmp = tmp->cdr.cell) {
                        if (tmp->car.cell != NULL && tmp->type == type_list) {
                                print_space(depth + 1, out);
                                print_string("(\n", out);

                                print_sexpr(tmp->car.cell, depth + 1, out, err);

                                print_space(depth + 1, out);
                                print_string(")\n", out);
                        }

                        if (tmp->type != type_list) {
                                print_sexpr(tmp, depth + 1, out, err);
                        }
                }
                if (depth == 0) {
                        print_string(")\n", out);
                }
        }

        return;
}

void free_sexpr(cell_t * list, file_io_t * err)
{
        cell_t *tmp,*free_me;
        if (list == NULL)
                return;
        if (list->type == type_str) {
                free(list->car.s);
                return;
        } else if (list->type == type_symbol) {
                free(list->car.s);
                return;
        } else if (list->type == type_list) {
                for (tmp = list; tmp != NULL; ) {
                        if (tmp->car.cell != NULL && tmp->type == type_list) {
                                free_sexpr(tmp->car.cell, err);
                        }

                        if (tmp->type != type_list) {
                                free_sexpr(tmp, err);
                        }

                        free_me=tmp;
                        tmp = tmp -> cdr.cell;
                        free(free_me);
                }
                return;
        }

        return;
}
