/**@file  bignum.c
 * @brief An arbitrary precision arithmetic library
 * @todo  Finish library; make more efficient, convert other bases to and
 *        from internal base, use an internal base that is a power of 2, not
 *        10. Fix memory leaks. **/

#include "bignum.h"
#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#define INTERNAL_BASE      (10)
#define BIGNUM_DEFAULT_LEN (64)
#define MAX_BIGNUM_STR     (BIGNUM_DEFAULT_LEN + 2)	/* - sign and null termination */
#define MAX(X, Y)          ((X) > (Y) ? (X) : (Y))

struct bignum {
	uint8_t *digits;	/* the actual bignum */
	size_t lastdigit;	/* last digit of the number */
	size_t allocated;	/* current size of digits[] */
	int isnegative;	/* 1 == negative, 0 == positive */
};

static uint8_t binlog2(size_t v) {
	uint8_t r = 0;
	while (v >>= 1)
		 r++;
	return r;
}

/************************ bignum library **************************************/

static void adjust_last(bignum * n) {
/*clean up after operation */
	assert(n);
	while ((n->lastdigit > 0) && (n->digits[n->lastdigit] == 0))
		n->lastdigit--;
	if ((n->lastdigit == 0) && (n->digits[0] == 0))
		n->isnegative = 0;
}

static int leftshift(bignum * n, unsigned d) {	/*Multiply a bignum by radix^d */
	size_t i;
	uint8_t *p;
	assert(n);

	if ((0 == n->lastdigit) && (0 == n->digits[0]))
		return 0;
	if (!(p = realloc(n->digits, n->allocated + d)))
		return -1;

	n->digits = p;
	n->allocated += d;
	i = n->lastdigit + 1;

	do {
		i--;
		n->digits[i + d] = n->digits[i];
	} while (i > 0);

	for (i = 0; i < d; i++)
		n->digits[i] = 0;
	n->lastdigit = n->lastdigit + d;
	return 0;
}

bignum *bignum_strtobig(const char *s, unsigned int base) {
	size_t i = 0;
	size_t len;
	bignum *n;

	assert(10 == base);
		    /** @warning temporary measure **/

	if (!s || (MAX_RADIX < base))
		return NULL;

	len = strlen(s);

	if (!(n = bignum_create(0, len + 1)))
		return NULL;

	if ('-' == s[0]) {
		i++;
		n->isnegative = 1;
	} else if ('+' == s[0]) {
		i++;
	}

/** @todo convert all bases between 1 and 17 **/
	while ('\0' != s[i]) {
		if (!isdigit(s[i]))
			return NULL;
		n->digits[len - i - 1] = (uint8_t) (s[i] - '0');
		n->lastdigit++;
		i++;
	}

	adjust_last(n);
	return n;
}

char *bignum_bigtostr(bignum * n, unsigned int base) {
	size_t allocate = 0, i = 0;
	char *s;
	if ((MAX_RADIX < base) || (base <= 1) || !n)
		return NULL;
	/* +3 comes from '\0', possible +/- and array size +1 */
	allocate = n->lastdigit * ((sizeof(n->digits[0]) * 8) / binlog2(base)) + 3;

	if (!(s = calloc(allocate, sizeof(*s))))
		return NULL;

	if (1 == n->isnegative)
		s[0] = '-';

	for (i = 0; i <= n->lastdigit; i++) {
		size_t index = (n->lastdigit - i) + n->isnegative;
		s[index] = '0' + n->digits[i];
	}
	return s;
}

bignum *bignum_create(int initialize_to, size_t len) {
	size_t x = 0;
	bignum *n = calloc(sizeof(*n), 1);

	if (!n)
		return NULL;

	/* assert(16 == INTERNAL_BASE); */
	n->digits = calloc(len, sizeof(n->digits[0]));
	n->allocated = len;

	if (!n->digits) {
		free(n);
		return NULL;
	}
	memset(n->digits, 0, len);

	n->isnegative = 0 > initialize_to ? 1 : 0;	/* x = 0 > a */

	if (0 == initialize_to) {
		n->lastdigit = 0;
		return n;
	}

	n->lastdigit = -1;
	x = (size_t) abs(initialize_to);

	do {
		n->lastdigit++;
		n->digits[n->lastdigit] = x % INTERNAL_BASE;
		x = x / INTERNAL_BASE;
	} while (0 < x);
	return n;
}

void bignum_destroy(bignum * n) {
	if (!n)
		return;
	free(n->digits);
	n->digits = NULL;
	free(n);
}

int bignum_compare(bignum * a, bignum * b) {
	size_t i = 0;
	assert(a && b);

	if ((a->isnegative == 1) && (b->isnegative == 0))
		return -1;
	if ((a->isnegative == 0) && (b->isnegative == 1))
		return 1;

	if (a->lastdigit < b->lastdigit)
		return a->isnegative ? 1 : -1;
	if (a->lastdigit > b->lastdigit)
		return a->isnegative ? -1 : 1;

	i = a->lastdigit + 1;
	do {
		i--;
		if (a->digits[i] < b->digits[i])
			return a->isnegative ? 1 : -1;
		if (a->digits[i] > b->digits[i])
			return a->isnegative ? -1 : 1;
	} while (i > 0);
	return 0;
}

bignum *bignum_add(bignum * a, bignum * b) {
	uint16_t carry = 0;
	size_t i = 0, allocate;
	bignum *result;
	int isnegative = 0;
	assert(a && b);

	if (a->isnegative == b->isnegative) {
		isnegative = a->isnegative;
	} else {
		if (a->isnegative == 1) {
			a->isnegative = 0;
			result = bignum_subtract(b, a);
			a->isnegative = 1;
			return result;
		} else if (b->isnegative == 1) {
			b->isnegative = 0;
			result = bignum_subtract(a, b);
			b->isnegative = 1;
			return result;
		}
	}

	allocate = MAX(a->lastdigit, b->lastdigit) + 2;

	if (!(result = bignum_create(0, allocate)))
		return NULL;

	result->isnegative = isnegative;
	result->lastdigit = MAX(a->lastdigit, b->lastdigit) + 1;

	for (i = 0; i <= result->lastdigit; i++) {
		uint16_t j, ai, bi;
		ai = i > a->lastdigit ? 0 : a->digits[i];
		bi = i > b->lastdigit ? 0 : b->digits[i];
		j = carry + ai + bi;
		result->digits[i] = j % INTERNAL_BASE;
		carry = j / INTERNAL_BASE;
	}
	adjust_last(result);
	return result;
}

bignum *bignum_subtract(bignum * a, bignum * b) {
	int borrow = 0;
	size_t i = 0;
	bignum *result;
	assert(a && b);

	if ((1 == a->isnegative) || (1 == b->isnegative)) {
		b->isnegative = b->isnegative == 1 ? 0 : 1;
		result = bignum_add(a, b);
		b->isnegative = b->isnegative == 1 ? 0 : 1;
		return result;
	}

	if (-1 == bignum_compare(a, b)) {
		result = bignum_subtract(b, a);
		result->isnegative = 1;
		return result;
	}

	if (!(result = bignum_create(0, a->lastdigit + 1)))
		return NULL;

	result->lastdigit = MAX(a->lastdigit, b->lastdigit);

	for (i = 0; i <= result->lastdigit; i++) {
		int16_t x, ai, bi;
		ai = i > a->lastdigit ? 0 : a->digits[i];
		bi = i > b->lastdigit ? 0 : b->digits[i];
		x = ai - borrow - bi;

		borrow = 0 < a->digits[i] ? 0 : borrow;
		if (0 > x) {
			x += INTERNAL_BASE;
			borrow = 1;
		}
		result->digits[i] = x % INTERNAL_BASE;
	}

	adjust_last(result);
	return result;
}

bignum *bignum_multiply(bignum * a, bignum * b) {
	bignum *row, *tmp, *result = NULL;
	size_t i, j, allocate;
	assert(a && b);

	allocate = (2 * MAX(a->lastdigit, b->lastdigit)) + 1;

	if (!(result = bignum_create(0, allocate)))
		return NULL;
	if (!(row = bignum_create(0, allocate)))
		return NULL;

	bignum_copy(row, a);

	for (i = 0; i <= b->lastdigit; i++) {
		for (j = 0; j < b->digits[i]; j++) {
			tmp = result;
			if (!(result = bignum_add(result, row)))
				return NULL;
			bignum_destroy(tmp);
		}
		if (leftshift(row, 1) < 0)
			return NULL;
	}
	bignum_destroy(row);
	result->isnegative = a->isnegative ^ b->isnegative;
	adjust_last(result);
	return result;
}

bignum_div_t *bignum_divide(bignum * a, bignum * b) {
	bignum_div_t *result;
	bignum *row, *tmp, *quotient, *remainder;
	size_t i;
	int asign, bsign;
	assert(a && b);

	if (!(quotient = bignum_create(0, a->lastdigit + 1)))
		goto fail;
	if (!(row = bignum_create(0, a->lastdigit + 1)))
		goto free_quotient;

	/*quotient is zero at this point */
	if (bignum_compare(b, quotient) == 0)
		goto free_row;

	asign = a->isnegative;
	bsign = b->isnegative;

	quotient->isnegative = a->isnegative ^ b->isnegative;

	a->isnegative = 0;
	b->isnegative = 0;

	quotient->lastdigit = a->lastdigit;

	i = a->lastdigit + 1;
	do {
		i--;
		if (leftshift(row, 1) < 0)
			return NULL;
		row->digits[0] = a->digits[i];
		quotient->digits[i] = 0;
		while (-1 < bignum_compare(row, b)) {
			quotient->digits[i]++;
			if (!(tmp = bignum_subtract(row, b)))
				goto free_row;
			bignum_copy(row, tmp);
			bignum_destroy(tmp);
		}
	} while (i > 0);
	remainder = row;

	a->isnegative = asign;
	b->isnegative = bsign;

	adjust_last(quotient);
	adjust_last(remainder);

	if (!(result = malloc(sizeof(*result))))
		goto free_row;

	result->quotient = quotient;
	result->remainder = remainder;

	return result;
free_row:
	free(row);
free_quotient:
	free(quotient);
fail:
	return NULL;
}

void bignum_copy(bignum * dst, bignum * src) {
	assert(dst && src);

	if (!dst->digits) {
		free(dst->digits);
		dst->digits = NULL;
	}

	dst->digits = malloc(src->allocated * sizeof(dst->digits[0]));
	if (!(dst->digits)) {
		exit(EXIT_FAILURE);
	}
	assert(dst->digits);

	memmove(dst->digits, src->digits, sizeof(src->digits[0]) * (src->lastdigit + 1));

	dst->lastdigit = src->lastdigit;
	dst->allocated = src->allocated;
	dst->isnegative = src->isnegative;
	adjust_last(dst);
}

