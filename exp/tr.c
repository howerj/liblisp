/* TODO: compliment of set one, ranges, cleanup (make suitable for a tr library routine) */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <assert.h>
#include "util.h"

struct tr_state {
        bool set_squ[256], set_del[256], compliment, squeeze, delete;
        uint8_t set_tr[256], previous_char;
};

static int tr_getnext(uint8_t **s){
        uint8_t seq[5] = "0000";
        if((*s)[0] == '\0'){
        } else if((*s)[0] == '\\'){
                switch((*s)[1]){
                case 'a': (*s)+=2; return '\a';
                case 'b': (*s)+=2; return '\b';
                case 'f': (*s)+=2; return '\f';
                case 'n': (*s)+=2; return '\n';
                case 'r': (*s)+=2; return '\r';
                case 't': (*s)+=2; return '\t';
                case 'v': (*s)+=2; return '\v';
                case '-': (*s)+=2; return '-';
                case '\\': (*s)+=2; return '\\';
                case '\0': return -1;
                }
                if(strspn((char *)(*s)+1,"01234567")>2){
                        int r = strtol(memcpy(seq+1,(*s)+1,sizeof(seq)-1) - 1,NULL,8) & 0377;
                        *s+=4;
                        return r;
                }
        } else {
                return (*s)++,(*s-1)[0];
        }
        return -1;
}

static int tr_init(struct tr_state *tr, char *mode, uint8_t *s1, uint8_t *s2){
        unsigned int i = 0;
        int c,d,cp,dp;
        assert((NULL != tr) && (NULL !=  mode) && (NULL != s1)); /*s2 is optional*/
        while((c = mode[i++])){
                switch(c){
                        case 'x': break;
                        case 'c': tr->compliment = true; break;
                        case 's': tr->squeeze = true; break;
                        case 'd': tr->delete = true; break;
                        default:  fprintf(stderr,"'%c' invalid mode\n",c); return -1;
                }
        }

        for(i = 0; i < sizeof(tr->set_tr); i++)
                tr->set_tr[i] = i;

        if(true == tr->delete){
                if(NULL != s2){
                        fprintf(stderr,"set 2 should be zero in delete mode\n");
                        return -1;
                }
                while((dp = tr_getnext(&s1))>0)
                        tr->set_del[dp] = true;
                return 0;
        }

        c = d = -1;
        while((cp = tr_getnext(&s1)) > 0){
                dp = tr_getnext(&s2);
                if(((cp < 0) && (c < 0)) || ((dp < 0) && (d < 0)))
                        return -1;
                c = cp;
                d = dp;
                tr->set_tr[c] = d;
                if(tr->squeeze)
                        tr->set_squ[c] = true;
        }
        return 0;
}

static int tr_char(struct tr_state *tr, uint8_t c){ /*return character to emit, -1 otherwise*/
        assert(tr);
        if((c == tr->previous_char) && (true == tr->squeeze) && (true == tr->set_squ[c]))
                return -1;
        tr->previous_char = c;
        if(true == tr->delete)
                return tr->set_del[c] ? -1: c;
        return tr->set_tr[c];       
}

static void tr_file(struct tr_state *tr, FILE *input, FILE *output){
        int c;
        assert(tr && input && output);
        while(EOF != (c = fgetc(input)))
                if((c = tr_char(tr,c))>0)
                        fputc(c,output);
}

int main_tr(int argc, char **argv)
{
        struct tr_state tr;
        uint8_t *s1 = NULL, *s2 = NULL;
        char *mode = NULL, *prog;
        int i;
        memset(&tr,0,sizeof(tr));
        prog = argv[0], argc--, argv++;
        if(argc>=1) mode = argv[0], argc--,argv++;
        if(argc>=1) s1 = (uint8_t*)argv[0], argc--,argv++;
        if(argc>=1) s2 = (uint8_t*)argv[0], argc--,argv++;
        if((NULL == mode) || (NULL == s1) || (tr_init(&tr,mode,s1,s2) < 0))
                return fprintf(stderr,"usage: %s (x|c|s|d)+ set set?\n",prog),1;
        if(!argc)
                return tr_file(&tr,stdin,stdout),0;
        for(i = 0; i < argc; i++){
                FILE *input = fopen_or_fail(argv[i],"rb");
                tr_file(&tr,input,stdout);
                fclose(input);
        }
        return 0;
}
