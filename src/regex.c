/**
 *  @file           regex.c
 *  @brief          A limited regex parser
 *  @author         Richard Howe, See Note
 *  @license        Unknown
 *  @email          howe.r.j.89@gmail.com
 *
 *  This is a simple regex engine based on the code obtained from
 *  <http://www.cs.princeton.edu/courses/archive/spr09/cos333/beautiful.html>
 *  In a short article called ' A Regular Expression Matcher', the code
 *  was written by Rob Pike and the article by Brian Kernighan. The
 *  original code is quite beautiful, but I am responsible for butchering
 *  it into this version (Richard Howe).
 *
 *  The regex operators are:
 *
 *   'c'    matches any literal character 'c'
 *   '.'    matches any single character
 *   '^'    matches the beginning of the input string
 *   '$'    matches the end of the input string
 *   '*'    matches zero or more occurrences of the previous character
 *   '?'    matches zero or one occurrences of the previous character
 *   '+'    matches one or more occurrences of the previous character
 *   '\'    escapes the next character so it is treated as a literal
 *
 *  This regex engine, while fine for the moment, will need to be rewritten
 *  and in accordance to the exercises presented to the reader in the
 *  original article.
 *
 *  The API is very simple, there is only one function regex_match.
 *
 **/

#include "regex.h"

static regex_e matchhere(char *regexp, char *text, unsigned int depth);
static regex_e matchstar(bool literal, int c, char *regexp, char *text, unsigned int depth);

/**
 *  @brief          Search for regexp in text
 *  @param          regexp      The regular expression
 *  @param          text        The text to perform the match on
 *  @return         regex_e     See enum for description
 **/
regex_e regex_match(char *regexp, char *text)
{
        unsigned int depth = 0;
        if (regexp[0] == '^')
                return matchhere(regexp + 1, text, depth + 1);
        do {                    /* must look even if string is empty */
                if (matchhere(regexp, text, depth + 1))
                        return REGEX_MATCH_E;
        } while (*text++ != '\0');
        return REGEX_NOMATCH_E;
}

/**
 *  @brief          Search for regexp at beginning of text
 *  @param          regexp      The regular expression
 *  @param          text        The text to perform the match on
 *  @param          depth       Current recursion depth
 *  @return         regex_e     See enum for description
 **/
static regex_e matchhere(char *regexp, char *text, unsigned int depth)
{
        if (REGEX_MAX_DEPTH < depth)
                return REGEX_FAIL_E;
 BEGIN:
        if (regexp[0] == '\0')
                return REGEX_MATCH_E;
        if (regexp[0] == '\\' && regexp[1] == *text) {
                if (regexp[1] == *text) {
                        regexp += 2;
                        text++;
                        goto BEGIN;
                } else {
                        return REGEX_FAIL_E;
                }
        }
        if (regexp[1] == '?') {
                text = text + (regexp[0] == *text ? 1 : 0);
                regexp = regexp + 2;
                goto BEGIN;
        }
        if (regexp[1] == '+') {
                if (regexp[0] == '.' || regexp[0] == *text) {
                        return matchstar(false, regexp[0], regexp + 2, text, depth + 1);
                }
                return REGEX_NOMATCH_E;
        }
        if (regexp[1] == '*')
                return matchstar(false, regexp[0], regexp + 2, text, depth + 1);
        if (regexp[0] == '$' && regexp[1] == '\0')
                return *text == '\0';
        if (*text != '\0' && (regexp[0] == '.' || regexp[0] == *text)) {
                regexp++;
                text++;
                goto BEGIN;
        }
        return REGEX_NOMATCH_E;
}

/**
 *  @brief          Search for c*regexp at beginning of text
 *  @param          literal     'c' is a literal character not regex op           
 *  @param          c           character to match
 *  @param          regexp      The regular expression
 *  @param          text        The text to perform the match on
 *  @param          depth       Current recursion depth
 *  @return         regex_e     See enum for description
 **/
static regex_e matchstar(bool literal, int c, char *regexp, char *text, unsigned int depth)
{
        if (REGEX_MAX_DEPTH < depth)
                return REGEX_FAIL_E;
        do {                    /* a * matches zero or more instances */
                if (matchhere(regexp, text, depth + 1))
                        return REGEX_MATCH_E;
        } while (*text != '\0' && (*text++ == c || (c == '.' && !literal)));
        return REGEX_NOMATCH_E;
}

