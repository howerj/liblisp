/* http://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "regex.h"

#define BUFSZ (4096u)
char buf[BUFSZ];

void grep(char *regex, FILE * input)
{
  unsigned int i = 0;
  memset(buf, '\0', BUFSZ);
  while (fgets(buf, BUFSZ, input)) {
    i++;
    buf[strlen(buf)-1] = '\0'; /*replace newline*/
    if (match(regex, buf)) {
      printf("%d:\t%s\n", i, buf);
    }
    memset(buf, '\0', BUFSZ);
  }
}

int main(int argc, char **argv)
{
  FILE *input;
  unsigned int i;

  if (1 >= argc) {
    fprintf(stderr, "usage: %s <regex> <file>\n", argv[0]);
    exit(EXIT_FAILURE);
  }

  printf("regex:%s\n",argv[1]);
  if (2 == argc) {
    puts("file:\tstdin");
    input = stdin;

    grep(argv[1], input);

    fflush(NULL);
  } else {
    for (i = 2; i < argc; i++) {
      if (NULL == (input = fopen(argv[i], "r"))) {
        fprintf(stderr, "Error: could not open <%s> for reading.\n", argv[i]);
        exit(EXIT_FAILURE);
      } else {
        printf("file:\t%s\n", argv[i]);
      }

      grep(argv[1], input);

      fflush(NULL);
      fclose(input);

    }
  }
  return EXIT_SUCCESS;
}
