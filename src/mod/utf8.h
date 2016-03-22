/** @file    utf8.h
 *  @brief   UTF-8 decoder and validator, header
 *  @author  Bjoern Hoehrmann
 *  @license MIT
 *  @email   bjoern@hoehrmann.de */
 
#ifndef UTF8_H
#define UTF8_H

#include <stdarg.h>
#include <stdint.h>
#include <stddef.h>

enum {
	UTF8_ACCEPT,
	UTF8_REJECT
};

/* is c the start of a utf8 sequence? */
#define isutf(c) (((c)&0xC0)!=0x80)

uint32_t utf8_validate(char *str, size_t len);

/* convert UTF-8 data to wide character */
int utf8_toucs(uint32_t *dest, int sz, char *src, int srcsz);

/* the opposite conversion */
int utf8_toutf8(char *dest, int sz, uint32_t *src, int srcsz);

/* single character to UTF-8 */
int utf8_wc_toutf8(char *dest, uint32_t ch);

/* character number to byte offset */
int utf8_offset(char *str, int charnum);

/* byte offset to character number */
int utf8_charnum(char *s, int offset);

/* return next character, updating an index variable */
uint32_t utf8_nextchar(char *s, int *i);

/* move to next character */
void utf8_inc(char *s, int *i);

/* move to previous character */
void utf8_dec(char *s, int *i);

/* returns length of next utf-8 sequence */
int utf8_seqlen(char *s);

/* assuming src points to the character after a backslash, read an
   escape sequence, storing the result in dest and returning the number of
   input characters processed */
int utf8_read_escape_sequence(char *src, uint32_t *dest);

/* given a wide character, convert it to an ASCII escape sequence stored in
   buf, where buf is "sz" bytes. returns the number of characters output. */
int utf8_escape_wchar(char *buf, int sz, uint32_t ch);

/* convert a string "src" containing escape sequences to UTF-8 */
int utf8_unescape(char *buf, int sz, char *src);

/* convert UTF-8 "src" to ASCII with escape sequences.
   if escape_quotes is nonzero, quote characters will be preceded by
   backslashes as well. */
int utf8_escape(char *buf, int sz, char *src, int escape_quotes);

/* utility predicates used by the above */
int octal_digit(char c);
int hex_digit(char c);

/* return a pointer to the first occurrence of ch in s, or NULL if not
   found. character index of found character returned in *charn. */
char *utf8_strchr(char *s, uint32_t ch, int *charn);

/* same as the above, but searches a buffer of a given size instead of
   a NUL-terminated string. */
char *utf8_memchr(char *s, uint32_t ch, size_t sz, int *charn);

/* count the number of characters in a UTF-8 string */
int utf8_strlen(char *s);

int utf8_is_locale_utf8(char *locale);

#endif

