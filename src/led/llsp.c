
#include <ctype.h> /*isspace*/
#include <string.h> /*strlen*/
#include "type.h"
#include "lisp.h"
#include "linenoise.h"

#define MAX_AUTO_COMPLETE_STR_LEN (256)
#define GENERIC_BUF_LEN (256)

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

/**
 * @brief Parse a C-string counting the parentheses, accounting
 *        for embedded strings and escape chars. We increment
 *        the return value for a '(' and decrement it for a ')'
 * @param       line    line to count parentheses in
 * @return      int     0 means
 *
 */
int count_parens(char *line){
        int count = 0;

        for(; '\0' != *line; line++){
                switch(*line){
                case '(':
                        count++;
                        break;
                case ')':
                        count--;
                        break;
                case '\\': /*consume escaped chars*/
                        if('\0' == *(line+1))
                                return count;
                        line++;
                        break;
                case '"': /*consume strings*/
                        if('\0' == *(++line))
                                return count;
                        for(; '\0' != *line; line++){
                                if('"' == *line)
                                        break;

                                if('\\' == *line){
                                        if('\0' == *(line+1))
                                                return count;
                                        else 
                                                line++;
                                }

                        }
                        break;
                default:
                        break;
                }
        }

        return count;
}

int main(void){
        char *line = NULL, *statement = NULL;;
        int paren_count = 0, line_count = 0;;
        lisp l;

        linenoise_set_completion_callback(completion);
        linenoise_history_load(hist_file);
        linenoise_vi_mode(1);

        l = lisp_init();

        statement = calloc(GENERIC_BUF_LEN, sizeof(*statement));
        while ((line = linenoise(line_count?"      ":"llsp> ")) != NULL){
                if('\0' != line){
                        size_t allocate = 0;
                        paren_count += count_parens(line);

                        /*check for unbalanced parens*/
                        if(paren_count < 0 && line_count == 0){
                                fprintf(stderr,"Too many ')' not enough '('.\n");
                                free(line);
                                paren_count = 0;
                                line = NULL;
                                continue;
                        }

                        /*more core for the extra line*/
                        allocate = strlen(line) + 1;
                        allocate += (NULL==statement)? 0 : strlen(statement);
                        if(NULL == (statement = realloc(statement,allocate))){
                                fprintf(stderr,"realloc failed\n");
                                return EXIT_FAILURE;
                        }

                        strcat(statement,line);

                        /*need a new line because we do not have a complete
                         *statement*/
                        if(0 != paren_count){
                                free(line);
                                line = NULL;
                                line_count++;
                                continue;
                        }

                        linenoise_history_add(statement);      
                        linenoise_history_save(hist_file);

                        io_string_in(l->i,statement);
                        lisp_repl(l);

                        memset(statement, '\0', allocate);
                        paren_count = line_count = 0;
                }

                free(line);
                line = NULL;
        }
        free(statement);
        lisp_end(l);
        return 0;
}
