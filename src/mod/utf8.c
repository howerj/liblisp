/** @file    utf8.c
 *  @brief   UTF-8 decoder and validator
 *  @author  Bjoern Hoehrmann
 *  @author  Jeff Bezanson
 *  @license MIT
 *  @email   bjoern@hoehrmann.de
 *
 * This file is a mixture of MIT licensed code (by Bjoern Hoehrmann) and
 * public domain code (by Jeff Bezanson). Available from:
 *
 * http://bjoern.hoehrmann.de/utf-8/decoder/dfa/
 * and
 * http://www.cprogramming.com/tutorial/unicode.html
 *
 * Copyright (c) 2008-2009 Bjoern Hoehrmann <bjoern@hoehrmann.de>
 * See http://bjoern.hoehrmann.de/utf-8/decoder/dfa/ for details.
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.*/

#include "utf8.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#include "utf8.h"

static const uint8_t utf8d[] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 00..1f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 20..3f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 40..5f */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,	/* 60..7f */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9, 9,	/* 80..9f */
	7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7,	/* a0..bf */
	8, 8, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,	/* c0..df */
	0xa, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x3, 0x4, 0x3, 0x3,	/* e0..ef */
	0xb, 0x6, 0x6, 0x6, 0x5, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8, 0x8,	/* f0..ff */
	0x0, 0x1, 0x2, 0x3, 0x5, 0x8, 0x7, 0x1, 0x1, 0x1, 0x4, 0x6, 0x1, 0x1, 0x1, 0x1,	/* s0..s0 */
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1,	/* s1..s2 */
	1, 2, 1, 1, 1, 1, 1, 2, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1,	/* s3..s4 */
	1, 2, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1,	/* s5..s6 */
	1, 3, 1, 1, 1, 1, 1, 3, 1, 3, 1, 1, 1, 1, 1, 1, 1, 3, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,	/* s7..s8 */
};

static const uint32_t offsets_from_utf8[6] = {
	0x00000000UL, 0x00003080UL, 0x000E2080UL,
	0x03C82080UL, 0xFA082080UL, 0x82082080UL
};

static const char trailing_bytes_for_utf8[256] = {
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
	2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5
};

/*uint32_t decode(uint32_t* state, uint32_t* codep, uint32_t byte)
{
	uint32_t type = utf8d[byte];

	*codep = (*state != UTF8_ACCEPT) ?
		(byte & 0x3fu) | (*codep << 6) :
		(0xff >> type) & (byte);

	*state = utf8d[256 + *state*16 + type];
	return *state;
}*/

uint32_t utf8_validate(char *str, size_t len)
{
	size_t i;
	uint32_t type, state = UTF8_ACCEPT;

	for (i = 0; i < len; i++) {
		/* We don't care about the codepoint, so this is
		   a simplified version of the decode function. */
		type = utf8d[(uint8_t) str[i]];
		state = utf8d[256 + (state) * 16 + type];

		if (state == UTF8_REJECT)
			break;
	}

	return state;
}

/* returns length of next utf-8 sequence */
int utf8_seqlen(char *s)
{
	return trailing_bytes_for_utf8[(unsigned char)s[0]] + 1;
}

/* conversions without error checking
   only works for valid UTF-8, i.e. no 5- or 6-byte sequences
   srcsz = source size in bytes, or -1 if 0-terminated
   sz = dest size in # of wide characters

   returns # characters converted
   dest will always be L'\0'-terminated, even if there isn't enough room
   for all the characters.
   if sz = srcsz+1 (i.e. 4*srcsz+4 bytes), there will always be enough space.
*/
int utf8_toucs(uint32_t * dest, int sz, char *src, int srcsz)
{
	uint32_t ch;
	char *src_end = src + srcsz;
	int nb;
	int i = 0;

	while (i < sz - 1) {
		nb = trailing_bytes_for_utf8[(unsigned char)*src];
		if (srcsz == -1) {
			if (*src == 0)
				goto done_toucs;
		} else {
			if (src + nb >= src_end)
				goto done_toucs;
		}
		ch = 0;
		switch (nb) {
			/* these fall through deliberately */
		case 3:
			ch += (unsigned char)*src++;
			ch <<= 6;
		case 2:
			ch += (unsigned char)*src++;
			ch <<= 6;
		case 1:
			ch += (unsigned char)*src++;
			ch <<= 6;
		case 0:
			ch += (unsigned char)*src++;
		}
		ch -= offsets_from_utf8[nb];
		dest[i++] = ch;
	}
 done_toucs:
	dest[i] = 0;
	return i;
}

/* srcsz = number of source characters, or -1 if 0-terminated
   sz = size of dest buffer in bytes

   returns # characters converted
   dest will only be '\0'-terminated if there is enough space. this is
   for consistency; imagine there are 2 bytes of space left, but the next
   character requires 3 bytes. in this case we could NUL-terminate, but in
   general we can't when there's insufficient space. therefore this function
   only NUL-terminates if all the characters fit, and there's space for
   the NUL as well.
   the destination string will never be bigger than the source string.
*/
int utf8_toutf8(char *dest, int sz, uint32_t * src, int srcsz)
{
	uint32_t ch;
	int i = 0;
	char *dest_end = dest + sz;

	while (srcsz < 0 ? src[i] != 0 : i < srcsz) {
		ch = src[i];
		if (ch < 0x80) {
			if (dest >= dest_end)
				return i;
			*dest++ = (char)ch;
		} else if (ch < 0x800) {
			if (dest >= dest_end - 1)
				return i;
			*dest++ = (ch >> 6) | 0xC0;
			*dest++ = (ch & 0x3F) | 0x80;
		} else if (ch < 0x10000) {
			if (dest >= dest_end - 2)
				return i;
			*dest++ = (ch >> 12) | 0xE0;
			*dest++ = ((ch >> 6) & 0x3F) | 0x80;
			*dest++ = (ch & 0x3F) | 0x80;
		} else if (ch < 0x110000) {
			if (dest >= dest_end - 3)
				return i;
			*dest++ = (ch >> 18) | 0xF0;
			*dest++ = ((ch >> 12) & 0x3F) | 0x80;
			*dest++ = ((ch >> 6) & 0x3F) | 0x80;
			*dest++ = (ch & 0x3F) | 0x80;
		}
		i++;
	}
	if (dest < dest_end)
		*dest = '\0';
	return i;
}

int utf8_wc_toutf8(char *dest, uint32_t ch)
{
	if (ch < 0x80) {
		dest[0] = (char)ch;
		return 1;
	}
	if (ch < 0x800) {
		dest[0] = (ch >> 6) | 0xC0;
		dest[1] = (ch & 0x3F) | 0x80;
		return 2;
	}
	if (ch < 0x10000) {
		dest[0] = (ch >> 12) | 0xE0;
		dest[1] = ((ch >> 6) & 0x3F) | 0x80;
		dest[2] = (ch & 0x3F) | 0x80;
		return 3;
	}
	if (ch < 0x110000) {
		dest[0] = (ch >> 18) | 0xF0;
		dest[1] = ((ch >> 12) & 0x3F) | 0x80;
		dest[2] = ((ch >> 6) & 0x3F) | 0x80;
		dest[3] = (ch & 0x3F) | 0x80;
		return 4;
	}
	return 0;
}

/* charnum => byte offset */
int utf8_offset(char *str, int charnum)
{
	int offs = 0;

	while (charnum > 0 && str[offs]) {
		(void)(isutf(str[++offs]) || isutf(str[++offs]) || isutf(str[++offs]) || ++offs);
		charnum--;
	}
	return offs;
}

/* byte offset => charnum */
int utf8_charnum(char *s, int offset)
{
	int charnum = 0, offs = 0;

	while (offs < offset && s[offs]) {
		(void)(isutf(s[++offs]) || isutf(s[++offs]) || isutf(s[++offs]) || ++offs);
		charnum++;
	}
	return charnum;
}

/* number of characters */
int utf8_strlen(char *s)
{
	int count = 0;
	int i = 0;

	while (utf8_nextchar(s, &i) != 0)
		count++;

	return count;
}

/* reads the next utf-8 sequence out of a string, updating an index */
uint32_t utf8_nextchar(char *s, int *i)
{
	uint32_t ch = 0;
	int sz = 0;

	do {
		ch <<= 6;
		ch += (unsigned char)s[(*i)++];
		sz++;
	} while (s[*i] && !isutf(s[*i]));
	ch -= offsets_from_utf8[sz - 1];

	return ch;
}

void utf8_inc(char *s, int *i)
{
	(void)(isutf(s[++(*i)]) || isutf(s[++(*i)]) || isutf(s[++(*i)]) || ++(*i));
}

void utf8_dec(char *s, int *i)
{
	(void)(isutf(s[--(*i)]) || isutf(s[--(*i)]) || isutf(s[--(*i)]) || --(*i));
}

int octal_digit(char c)
{
	return (c >= '0' && c <= '7');
}

int hex_digit(char c)
{
	return ((c >= '0' && c <= '9') || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f'));
}

/* assumes that src points to the character after a backslash
   returns number of input characters processed */
int utf8_read_escape_sequence(char *str, uint32_t * dest)
{
	uint32_t ch;
	char digs[9] = "\0\0\0\0\0\0\0\0";
	int dno = 0, i = 1;

	ch = (uint32_t) str[0];	/* take literal character */
	if (str[0] == 'n')
		ch = L'\n';
	else if (str[0] == 't')
		ch = L'\t';
	else if (str[0] == 'r')
		ch = L'\r';
	else if (str[0] == 'b')
		ch = L'\b';
	else if (str[0] == 'f')
		ch = L'\f';
	else if (str[0] == 'v')
		ch = L'\v';
	else if (str[0] == 'a')
		ch = L'\a';
	else if (octal_digit(str[0])) {
		i = 0;
		do {
			digs[dno++] = str[i++];
		} while (octal_digit(str[i]) && dno < 3);
		ch = strtol(digs, NULL, 8);
	} else if (str[0] == 'x') {
		while (hex_digit(str[i]) && dno < 2) {
			digs[dno++] = str[i++];
		}
		if (dno > 0)
			ch = strtol(digs, NULL, 16);
	} else if (str[0] == 'u') {
		while (hex_digit(str[i]) && dno < 4) {
			digs[dno++] = str[i++];
		}
		if (dno > 0)
			ch = strtol(digs, NULL, 16);
	} else if (str[0] == 'U') {
		while (hex_digit(str[i]) && dno < 8) {
			digs[dno++] = str[i++];
		}
		if (dno > 0)
			ch = strtol(digs, NULL, 16);
	}
	*dest = ch;

	return i;
}

/* convert a string with literal \uxxxx or \Uxxxxxxxx characters to UTF-8
   example: utf8_unescape(mybuf, 256, "hello\\u220e")
   note the double backslash is needed if called on a C string literal */
int utf8_unescape(char *buf, int sz, char *src)
{
	int c = 0, amt;
	uint32_t ch;
	char temp[4];

	while (*src && c < sz) {
		if (*src == '\\') {
			src++;
			amt = utf8_read_escape_sequence(src, &ch);
		} else {
			ch = (uint32_t) * src;
			amt = 1;
		}
		src += amt;
		amt = utf8_wc_toutf8(temp, ch);
		if (amt > sz - c)
			break;
		memcpy(&buf[c], temp, amt);
		c += amt;
	}
	if (c < sz)
		buf[c] = '\0';
	return c;
}

int utf8_escape_wchar(char *buf, int sz, uint32_t ch)
{
	if (ch == L'\n')
		return snprintf(buf, sz, "\\n");
	else if (ch == L'\t')
		return snprintf(buf, sz, "\\t");
	else if (ch == L'\r')
		return snprintf(buf, sz, "\\r");
	else if (ch == L'\b')
		return snprintf(buf, sz, "\\b");
	else if (ch == L'\f')
		return snprintf(buf, sz, "\\f");
	else if (ch == L'\v')
		return snprintf(buf, sz, "\\v");
	else if (ch == L'\a')
		return snprintf(buf, sz, "\\a");
	else if (ch == L'\\')
		return snprintf(buf, sz, "\\\\");
	else if (ch < 32 || ch == 0x7f)
		return snprintf(buf, sz, "\\x%hhX", (unsigned char)ch);
	else if (ch > 0xFFFF)
		return snprintf(buf, sz, "\\U%.8X", (uint32_t) ch);
	else if (ch >= 0x80 && ch <= 0xFFFF)
		return snprintf(buf, sz, "\\u%.4hX", (unsigned short)ch);

	return snprintf(buf, sz, "%c", (char)ch);
}

int utf8_escape(char *buf, int sz, char *src, int escape_quotes)
{
	int c = 0, i = 0, amt;

	while (src[i] && c < sz) {
		if (escape_quotes && src[i] == '"') {
			amt = snprintf(buf, sz - c, "\\\"");
			i++;
		} else {
			amt = utf8_escape_wchar(buf, sz - c, utf8_nextchar(src, &i));
		}
		c += amt;
		buf += amt;
	}
	if (c < sz)
		*buf = '\0';
	return c;
}

char *utf8_strchr(char *s, uint32_t ch, int *charn)
{
	int i = 0, lasti = 0;
	uint32_t c;

	*charn = 0;
	while (s[i]) {
		c = utf8_nextchar(s, &i);
		if (c == ch) {
			return &s[lasti];
		}
		lasti = i;
		(*charn)++;
	}
	return NULL;
}

char *utf8_memchr(char *s, uint32_t ch, size_t sz, int *charn)
{
	size_t i = 0, lasti = 0;
	uint32_t c;
	int csz;

	*charn = 0;
	while (i < sz) {
		c = csz = 0;
		do {
			c <<= 6;
			c += (unsigned char)s[i++];
			csz++;
		} while (i < sz && !isutf(s[i]));
		c -= offsets_from_utf8[csz - 1];

		if (c == ch) {
			return &s[lasti];
		}
		lasti = i;
		(*charn)++;
	}
	return NULL;
}

int utf8_is_locale_utf8(char *locale)
{
	/* this code based on libutf8 */
	const char *cp = locale;

	for (; *cp != '\0' && *cp != '@' && *cp != '+' && *cp != ','; cp++) {
		if (*cp == '.') {
			const char *encoding = ++cp;
			for (; *cp != '\0' && *cp != '@' && *cp != '+' && *cp != ','; cp++) ;
			if ((cp - encoding == 5 && !strncmp(encoding, "UTF-8", 5))
			    || (cp - encoding == 4 && !strncmp(encoding, "utf8", 4)))
				return 1;	/* it's UTF-8 */
			break;
		}
	}
	return 0;
}

/*
#ifdef WIN32
#include <malloc.h>
#else
#include <alloca.h>
#endif

 int utf8_vprintf(char *fmt, va_list ap)
{
    int cnt, sz=0;
    char *buf;
    uint32_t *wcs;

    sz = 512;
    buf = (char*)alloca(sz);
 try_print:
    cnt = vsnprintf(buf, sz, fmt, ap);
    if (cnt >= sz) {
        buf = (char*)alloca(cnt - sz + 1);
        sz = cnt + 1;
        goto try_print;
    }
    wcs = (uint32_t*)alloca((cnt+1) * sizeof(uint32_t));
    cnt = utf8_toucs(wcs, cnt+1, buf, cnt);
    printf("%ls", (wchar_t*)wcs);
    return cnt;
}

int utf8_printf(char *fmt, ...)
{
    int cnt;
    va_list args;

    va_start(args, fmt);

    cnt = utf8_vprintf(fmt, args);

    va_end(args);
    return cnt;
}*/
