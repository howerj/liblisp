/**
 *  @file           bignum.c
 *  @brief          Bignum library functions, header 
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v2.0 or later version
 *  @email          howe.r.j.89@gmail.com
 * 
 **/

#ifndef BIGNUM_H
#define BIGNUM_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>

#define MAX_RADIX       (16)

struct bignum;
typedef struct bignum bignum;

typedef struct{
        bignum *quotient;
        bignum *remainder;
} bignum_div_t;

bignum *bignum_strtobig(const char *s, unsigned int base);
char *bignum_bigtostr(bignum *n, unsigned int base);
bignum *bignum_create(int initialize_to, size_t len);
void bignum_destroy(bignum *n);
int bignum_compare(bignum *a, bignum *b);
bignum *bignum_add(bignum *a, bignum *b);
bignum *bignum_subtract(bignum *a, bignum *b);
bignum *bignum_multiply(bignum *a, bignum *b);
bignum_div_t* bignum_divide(bignum *a, bignum *b);
void bignum_copy(bignum *dst, bignum *src);

#ifdef __cplusplus
}
#endif
#endif  /* BIGNUM_H */
