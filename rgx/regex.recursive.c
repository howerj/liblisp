/* http://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html */

/*
 * NEW:
 *  Added escape character '\\'
 *  Added '?' for zero or one matches
 * NOTES:
 *  Escape char does not work for '*'
 *  '?' *seems* to work
 *  '+' *seems* to work
 */
#include "regex.h"
#include <stdio.h>

#define MAX_DEPTH (8192u)

static int matchhere(char *regexp, char *text, unsigned int depth);
static int matchstar(int c, char *regexp, char *text, unsigned int depth);

#define depthchk(DEPTH)  if(DEPTH>MAX_DEPTH){\
  fprintf(stderr,"depth exceeded!\n");\
  return 0;\
  }

/* match: search for regexp anywhere in text */
int match(char *regexp, char *text)
{
  unsigned int depth = 0;
  if (regexp[0] == '^')
    return matchhere(regexp + 1, text, depth+1);
  do {                          /* must look even if string is empty */
    if (matchhere(regexp, text,depth + 1))
      return 1;
  } while (*text++ != '\0');
  return 0;
}

/* matchhere: search for regexp at beginning of text */
int matchhere(char *regexp, char *text, unsigned int depth)
{
  depthchk(depth);
  if (regexp[0] == '\0')
    return 1;
  /*** new ************************************************/
  if (regexp[0] == '\\' && regexp[1] == *text){
    if(regexp[1] == *text)
      return matchhere(regexp + 2, text + 1, depth + 1);
    else 
      return 0;
  }
  if (regexp[1] == '?')
    return matchhere(regexp + 2, text + (regexp[0]==*text?1:0),depth + 1);
  if (regexp[1] == '+'){
    if(regexp[0] == '.' || regexp[0] == *text){
      return matchstar(regexp[0], regexp + 2, text, depth + 1);
    } 
    return 0;
  }
  /********************************************************/
  if (regexp[1] == '*')
    return matchstar(regexp[0], regexp + 2, text, depth + 1);
  if (regexp[0] == '$' && regexp[1] == '\0')
    return *text == '\0';
  if (*text != '\0' && (regexp[0] == '.' || regexp[0] == *text))
    return matchhere(regexp + 1, text + 1, depth + 1);
  return 0;
}

/* matchstar: search for c*regexp at beginning of text */
int matchstar(int c, char *regexp, char *text, unsigned int depth)
{
  depthchk(depth);
  do {                          /* a * matches zero or more instances */
    if (matchhere(regexp, text, depth + 1))
      return 1;
  } while (*text != '\0' && (*text++ == c || c == '.'));
  return 0;
}
