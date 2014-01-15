/** 
 * modified from 
 *  http://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html 
 *
 * @license Unknown?
 *
 *  TODO:
 *    * Limit stack depth
 *    * Change types used
 *    * Add + and ? and [abc]
 */

#include "regex.h"

static int matchhere(char *regexp, char *text, unsigned int depth);
static int matchstar(int c, char *regexp, char *text, unsigned int depth);

/* match: search for regexp anywhere in text */
int match(char *regexp, char *text)
{
  int ret;
  if ('^' == regexp[0])
    return matchhere(regexp + 1, text, 0);
  do {                          /* must look even if string is empty */
    if ((ret = matchhere(regexp, text, 0)))
      return ret;
  } while ('\0' != *text++);
  return 0;
}

/* matchhere: search for regexp at beginning of text */
int matchhere(char *regexp, char *text, unsigned int depth)
{
  if (depth > maxdepth_m)
    return -1;
  if ('\0' == regexp[0])
    return 1;
  if ('*' == regexp[1])
    return matchstar(regexp[0], regexp + 2, text, depth + 1);
  if (('$' == regexp[0]) && ('\0' == regexp[1]))
    return *text == '\0';
  if ('\0' != *text && (('.' == regexp[0]) || (regexp[0] == *text)))
    return matchhere(regexp + 1, text + 1, depth + 1);
  return 0;
}

/* matchstar: search for c*regexp at beginning of text */
int matchstar(int c, char *regexp, char *text, unsigned int depth)
{
  if (depth > maxdepth_m)
    return -1;
  do {                          /* a * matches zero or more instances */
    if (matchhere(regexp, text, depth + 1))
      return 1;
  } while (*text != '\0' && ((*text++ == c) || ('.' == c)));
  return 0;
}
