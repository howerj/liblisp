/** @file       crc.c
 *  @brief      CRC routines
 *  This code has been swiped from the PNG specification: 
 *  <http://www.libpng.org/pub/png/spec/1.2/PNG-CRCAppendix.html>
 *  It is a CRC-32 algorithm used in PNG, Ethernet, Gzip, ... **/
#include "crc.h"

static uint32_t crc_table[256];	/* Table of CRCs of all 8-bit messages. */
static int crc_table_computed = 0;	/* Flag: has the table been computed? */

static void make_crc_table(void)
{				/* Make the table for a fast CRC. */
	uint32_t c;
	int n, k;

	for (n = 0; n < 256; n++) {
		c = (uint32_t) n;
		for (k = 0; k < 8; k++) {
			if (c & 1)
				c = 0xedb88320L ^ (c >> 1);
			else
				c = c >> 1;
		}
		crc_table[n] = c;
	}
	crc_table_computed = 1;
}

   /* Update a running CRC with the bytes buf[0..len-1]--the CRC
      should be initialized to all 1's, and the transmitted value
      is the 1's complement of the final running CRC (see the
      crc() routine below)). */
uint32_t crc_update(uint32_t crc, uint8_t * abuf, size_t len)
{
	uint32_t c = crc;
	size_t n;

	if (!crc_table_computed)
		make_crc_table();
	for (n = 0; n < len; n++) {
		c = crc_table[(c ^ abuf[n]) & 0xff] ^ (c >> 8);
	}
	return c;
}

uint32_t crc(uint8_t * abuf, size_t len)
{				/* Return the CRC of the bytes abuf[0..len-1]. */
	return crc_update(0xffffffffL, abuf, len) ^ 0xffffffffL;
}

uint32_t crc_init(uint8_t * abuf, size_t len)
{
	return crc_update(0xffffffffL, abuf, len);
}

uint32_t crc_final(uint32_t crc)
{
	return crc ^ 0xffffffffL;
}
