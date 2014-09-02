#include <string.h> /*strlen*/
#include "type.h"
#include "lisp.h"
#include "linenoise.h"

static char *hist_file = "history.txt";

void completion(const char *buf, linenoise_completions * lc)
{
        if (buf[0] == 'h') {
                linenoise_add_completion(lc, "hello");
                linenoise_add_completion(lc, "hello there");
        }
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
