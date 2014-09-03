
#include <ctype.h> /*isspace*/
#include <string.h> /*strlen*/
#include "type.h"
#include "lisp.h"
#include "linenoise.h"

#define MAX_AUTO_COMPLETE_STR_LEN (256)

static char *hist_file = "history.txt";

void completion(const char *buf, linenoise_completions * lc)
{
        size_t i, buf_len = strlen(buf);
        char ident[MAX_AUTO_COMPLETE_STR_LEN] = "";
        char c = buf[0];

        for(i = buf_len; ((buf + i) != buf); i--){
                c = buf[i];
                if(isspace(c)||(c=='(')||(c==')')){
                        break;
                }
        }

        c = buf[i];
        if(isspace(c)||(c=='(')||(c==')')){
                i++;
        }

        if(i > MAX_AUTO_COMPLETE_STR_LEN) /*silently return*/
                return;

        memmove(ident, buf+i, buf_len-i);

        switch(ident[0]){
                case 'b':
                        linenoise_add_completion(lc, "begin");
                        break;
                case 'c':
                        linenoise_add_completion(lc, "car");
                        linenoise_add_completion(lc, "cdr");
                        linenoise_add_completion(lc, "cons");
                        break;
                case 'd':
                        linenoise_add_completion(lc, "define");
                        break;
                case 'i':
                        linenoise_add_completion(lc, "if");
                        break;
                case 'l':
                        linenoise_add_completion(lc, "lambda");
                        break;
                case 'n':
                        linenoise_add_completion(lc, "nil");
                        linenoise_add_completion(lc, "nth");
                        break;
                case 's':
                        linenoise_add_completion(lc, "scar");
                        linenoise_add_completion(lc, "scdr");
                        linenoise_add_completion(lc, "scons");
                        linenoise_add_completion(lc, "system");
                        linenoise_add_completion(lc, "set");
                        break;
                case 'q':
                        linenoise_add_completion(lc, "quote");
                        break;
                default:
                        return;
        }
        /*
        if (buf[0] == 'h') {
                linenoise_add_completion(lc, "hello");
                linenoise_add_completion(lc, "hello there");
        }*/
}

int main(void){
        char *line;
        lisp l;

        linenoise_set_completion_callback(completion);
        linenoise_history_load(hist_file);   

        l = lisp_init();

        while ((line = linenoise("llsp> ")) != NULL){
                if('\0' != line){
                        linenoise_history_add(line);      
                        linenoise_history_save(hist_file);

                        memset(l->i, 0, sizeof(*l->i));
                        l->i->type = string_in;
                        l->i->ptr.string = line;
                        l->i->max = strlen(line);
                        lisp_repl(l);
                }

                free(line);
                line = NULL;
        }
        lisp_end(l);
        return 0;
}
