/** @file       tr.c
 *  @brief      A set of functions that act like the "tr" utility. This is not
 *              unicode aware!
 *  @author     Richard Howe (2015)
 *  @license    LGPL v2.1 or Later
 *  @email      howe.r.j.89@gmail.com
 *  @todo       compliment of set one and ranges characters should be added*/
#include "liblisp.h"
#include "private.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>

static int tr_getnext(uint8_t **s){
        uint8_t seq[5] = "0000";
        if((*s)[0] == '\0'){
        } else if((*s)[0] == '\\'){
                switch((*s)[1]){
                case 'a':  (*s) += 2; return '\a';
                case 'b':  (*s) += 2; return '\b';
                case 'f':  (*s) += 2; return '\f';
                case 'n':  (*s) += 2; return '\n';
                case 'r':  (*s) += 2; return '\r';
                case 't':  (*s) += 2; return '\t';
                case 'v':  (*s) += 2; return '\v';
                case '-':  (*s) += 2; return '-';
                case '\\': (*s) += 2; return '\\';
                case '\0': return -1;
                }
                if(strspn((char *)(*s)+1,"01234567") > 2){
                        int r = strtol((char*)memcpy(seq+1,(*s)+1, sizeof(seq) - 1) - 1, NULL, 8) & 0377;
                        *s += 4;
                        return r;
                }
        } else {
                return (*s)++, (*s-1)[0];
        }
        return -1;
}

int tr_init(tr_state *tr, char *mode, uint8_t *s1, uint8_t *s2) {
        unsigned i = 0;
        int c, d, cp, dp;
        assert(tr && mode && s1); /*s2 is optional*/
        while((c = mode[i++]))
                switch(c){
                        case 'x': break;
                        case 'c': tr->compliment_seq = 1; break;
                        case 's': tr->squeeze_seq = 1; break;
                        case 'd': tr->delete_seq = 1; break;
                        case 't': tr->truncate_seq = 1; break;
                        default:  return TR_EINVAL;
                }

        for(i = 0; i < 256; i++)
                tr->set_tr[i] = i;

        if(tr->delete_seq) {
                if(s2 || tr->truncate_seq) /*set 2 should be NULL in delete mode*/
                        return TR_DELMODE;
                while((dp = tr_getnext(&s1)) > 0)
                        tr->set_del[dp] = 1;
                return TR_OK;
        }

        if(tr->truncate_seq) {
                size_t s1l = strlen((char*)s1), s2l = strlen((char*)s2);
                s1[MIN(s2l, s1l)] = '\0';
        }

        c = d = -1;
        while((cp = tr_getnext(&s1)) > 0){
                dp = tr_getnext(&s2);
                if(((cp < 0) && (c < 0)) || ((dp < 0) && (d < 0)))
                        return TR_EINVAL;
                c = cp;
                d = dp;
                tr->set_tr[c] = d;
                if(tr->squeeze_seq)
                        tr->set_squ[c] = 1;
        }
        return TR_OK;
}

int tr_char(tr_state *tr, uint8_t c) { /*return character to emit, -1 otherwise*/
        assert(tr);
        if((c == tr->previous_char) && tr->squeeze_seq && tr->set_squ[c])
                return -1;
        tr->previous_char = c;
        if(tr->delete_seq)
                return tr->set_del[c] ? -1: c;
        return tr->set_tr[c];       
}

size_t tr_block(tr_state *tr, uint8_t *in, uint8_t *out, size_t len) {
        int c;
        size_t i = 0, j = 0;
        for(; j < len; j++)
                if((c = tr_char(tr, in[j])) >= 0)
                        out[i++] = c;
        return i;
}

tr_state *tr_new(void) { return calloc(1, sizeof(tr_state)); }
void tr_delete(tr_state *st) { free(st); }

