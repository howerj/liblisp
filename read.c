/** @file       read.c
 *  @brief      An S-Expression parser for liblisp
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com 
 *  
 *  An S-Expression parser, it takes it's input from a generic input
 *  port that can be set up to read from a string or a file. It would
 *  be nice if this S-Expression parser could be made so parsing of
 *  different things could be turned on or off, such as strings. This
 *  parser needs rewriting as well, parts of it are quite ugly.
 *  **/
 

#include "liblisp.h"
#include "private.h"
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

static int comment(io *i) { /**@brief process a comment from I/O stream**/
        int c; 
        while(((c = io_getc(i)) > 0) && (c != '\n')); 
        return c; 
} 

static void add_char(lisp *l, char ch)  { /**< add a char to the token buffer*/
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

static char *new_token(lisp *l) { /**< allocate a new token*/
        char *s;
        l->buf[l->buf_used++] = '\0';
        if(!(s = lstrdup(l->buf))) HALT(l, "%s", "out of memory");
        return s;
}

static void ungettok(lisp *l, char *token) { /**< push back a token*/
        l->token = token; 
        l->ungettok = 1; 
}

static char *gettoken(lisp *l, io *i) { /**< lexer*/
        int ch, end = 0;
        l->buf_used = 0;
        if(l->ungettok)
                return l->ungettok = 0, l->token; 
        do {
                if((ch = io_getc(i)) == EOF) 
                        return NULL;
                if(ch == '#' || ch == ';') { comment(i); continue; } /*ugly*/
        } while(isspace(ch) || ch == '#' || ch == ';');
        add_char(l, ch);
        if(strchr("()\'\"", ch))
                return new_token(l);
        for(;;) {
                if((ch = io_getc(i)) == EOF)
                        end = 1;
                if(ch == '#' || ch == ';') { comment(i); continue; } /*ugly*/
                if(strchr("()\'\"", ch) || isspace(ch)) {
                        io_ungetc(ch, i);
                        return new_token(l);
                }
                if(end) return new_token(l);
                add_char(l, ch);
        }
}

static cell *readstring(lisp *l, io* i) { /**< handle parsing a string*/
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
                        return mk_str(l, new_token(l));
                add_char(l, ch);
        }
        return gsym_nil();
}

static cell *readlist(lisp *l, io *i);
cell *reader(lisp *l, io *i) { /*read in s-expr, this should be rewritten*/
        char *token = gettoken(l, i), *fltend = NULL;
        double flt; 
        cell *ret;
        if(!token) return NULL;
        switch(token[0]) {
        case ')':  free(token); 
                   RECOVER(l, "\"unmatched %s\"", "')");
        case '(':  free(token); return readlist(l, i);
        case '"':  free(token); return readstring(l, i);
        case '\'': free(token); return cons(l, gsym_quote(), cons(l, reader(l,i), gsym_nil()));
        default:   if(is_number(token)) {
                           ret = mk_int(l, strtol(token, NULL, 0));
                           free(token);
                           return ret;
                   }
                   if(is_fnumber(token)) {
                        flt = strtod(token, &fltend);
                        if(!fltend[0]) {
                                free(token);
                                return mk_float(l, flt);
                        }
                   }
                   ret = intern(l, token);
                   if(get_sym(ret) != token) free(token);
                   return ret;
        }
        return gsym_nil();
}

static cell *readlist(lisp *l, io *i) { /**< read in a list*/
        char *token = gettoken(l, i), *stok;
        cell *tmp;
        if(!token) return NULL;
        switch(token[0]){
        case ')': return free(token), gsym_nil();
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

