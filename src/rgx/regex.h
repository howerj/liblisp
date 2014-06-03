/* http://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html */

typedef enum{
  regex_fail = -1,
  regex_nomatch = 0,
  regex_match = 1
} regex_e;

typedef enum{
  false,
  true
} bool;

int match(char *regexp, char *text);
