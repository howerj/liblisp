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
#include <stdio.h>
#include <string.h>
#include "regex.h"

static int matchhere(char *regexp, char *text, unsigned int depth);
static int matchstar(bool literal, int c, char *regexp, char *text, unsigned int depth);

/* match: search for regexp anywhere in text */
int regex_match(char *regexp, char *text)
{
  unsigned int depth = 0;
  if (regexp[0] == '^')
    return matchhere(regexp + 1, text, depth+1);
  do { /* must look even if string is empty */
    if (matchhere(regexp, text,depth + 1))
      return 1;
  } while (*text++ != '\0');
  return 0;
}

/* matchhere: search for regexp at beginning of text */
int matchhere(char *regexp, char *text, unsigned int depth)
{
  if(REGEX_MAX_DEPTH < depth)
    return -1;
BEGIN:
  if (regexp[0] == '\0')
    return 1;
  if (regexp[0] == '\\' && regexp[1] == *text){
    if(regexp[1] == *text){
      regexp += 2;
      text++;
      goto BEGIN;
    } else {
      return 0;
    }
  }
  if (regexp[1] == '?'){
    text = text + (regexp[0]==*text?1:0);
    regexp = regexp + 2;
    goto BEGIN;
  }
  if (regexp[1] == '+'){
    if(regexp[0] == '.' || regexp[0] == *text){
      return matchstar(false, regexp[0], regexp + 2, text, depth + 1);
    } 
    return 0;
  }
  if (regexp[1] == '*')
    return matchstar(false,regexp[0], regexp + 2, text, depth + 1);
  if (regexp[0] == '$' && regexp[1] == '\0')
    return *text == '\0';
  if (*text != '\0' && (regexp[0] == '.' || regexp[0] == *text)){
    regexp++;
    text++;
    goto BEGIN;
  }
  return 0;
}

/* matchstar: search for c*regexp at beginning of text */
int matchstar(bool literal, int c, char *regexp, char *text, unsigned int depth)
{
  if(REGEX_MAX_DEPTH < depth)
    return -1;
  do { /* a * matches zero or more instances */
    if (matchhere(regexp, text, depth + 1))
      return 1;
  } while (*text != '\0' && (*text++ == c || (c == '.' && !literal)));
  return 0;
}

