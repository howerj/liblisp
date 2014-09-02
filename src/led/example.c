#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "linenoise.h"

void completion(const char *buf, linenoise_completions * lc)
{
        if (buf[0] == 'h') {
                linenoise_add_completion(lc, "hello");
                linenoise_add_completion(lc, "hello there");
        }
}

int main(int argc, char **argv)
{
        char *line;
        char *prgname = argv[0];

        /* Parse options, with --multiline we enable multi line editing. */
        while (argc > 1) {
                argc--;
                argv++;
                if (!strcmp(*argv, "--multiline")) {
                        linenoise_set_multiline(1);
                        printf("Multi-line mode enabled.\n");
                } else if (!strcmp(*argv, "--keycodes")) {
                        linenoise_print_keycodes();
                        exit(0);
                } else {
                        fprintf(stderr, "Usage: %s [--multiline] [--keycodes]\n", prgname);
                        exit(1);
                }
        }

        /* Set the completion callback. This will be called every time the
         * user uses the <tab> key. */
        linenoise_set_completion_callback(completion);

        /* Load history from file. The history file is just a plain text file
         * where entries are separated by newlines. */
        linenoise_history_load("history.txt");    /* Load the history at startup */

        /* Now this is the main loop of the typical linenoise-based application.
         * The call to linenoise() will block as long as the user types something
         * and presses enter.
         *
         * The typed string is returned as a malloc() allocated string by
         * linenoise, so the user needs to free() it. */
        while ((line = linenoise("hello> ")) != NULL) {
                /* Do something with the string. */
                if (line[0] != '\0' && line[0] != '/') {
                        printf("echo: '%s'\n", line);
                        linenoise_history_add(line);      /* Add to the history. */
                        linenoise_history_save("history.txt");    /* Save the history on disk. */
                } else if (!strncmp(line, "/historylen", 11)) {
                        /* The "/historylen" command will change the history len. */
                        int len = atoi(line + 11);
                        linenoise_history_set_maxlen(len);
                } else if (line[0] == '/') {
                        printf("Unreconized command: %s\n", line);
                }
                free(line);
        }
        return 0;
}
