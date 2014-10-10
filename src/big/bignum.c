/**
 *  @file           bignum.c
 *  @brief          Bignum library functions for basic arbitrary precision ops
 *  @author         Richard James Howe.
 *  @copyright      Copyright 2013 Richard James Howe.
 *  @license        GPL v2.0 or later version
 *  @email          howe.r.j.89@gmail.com
 *
 *  References:
 *
 *  Knuth The Art of Computer Programming. Vol. II, Chap. 4.3-4.4, 2ed.
 * 
 *  @todo Use 256 as internal base
 *  @todo Rewrite API to be consistent with my Lisp implementation.
 *  @todo Handle bases other than 10 in conversions and internally
 *  @todo This library is horrendously inefficient, but simple, it
 *        needs reworking so it is still readable but it is faster.
 *
 **/

#include <stdio.h>  /* printf - debugging only */
#include <stdlib.h> /* abs, abort */
#include <stdint.h> /* uintX_t */
#include <string.h> /* strlen */
#include <ctype.h>  /* isdigit */
#include <assert.h> /* assert */
#include "bignum.h"

#define MIN(X,Y) ((X) < (Y) ? (X) : (Y))
#define MAX(X,Y) ((X) > (Y) ? (X) : (Y))

#define INTERNAL_BASE      (10)
#define BIGNUM_DEFAULT_LEN (64)
#define MAX_BIGNUM_STR     (BIGNUM_DEFAULT_LEN + 2) /* - sign and null termination */

struct bignum{
        uint8_t *digits;  /* the actual bignum */
        size_t lastdigit; /* last digit of the number */
        size_t allocated; /* current size of digits[] */
        int isnegative;   /* 1 == negative, 0 == positive */
};

static void adjust_last(bignum * n);
static int leftshift(bignum *n, unsigned int d);
static uint8_t binlog(size_t v);

static void _check(int checkme, char *file, int line);
#define check(X) _check((X),__FILE__,__LINE__)

/**** Functions with external linkage ****************************************/

/** 
 *  @brief    Convert a string to a bignum
 *  @param    s         String that needs converting
 *  @param    base      Base to convert string from
 *  @return   bignum*   Initialized bignum from string
 */
bignum *bignum_strtobig(const char *s, unsigned int base){
        size_t i = 0;
        size_t len;
        bignum *n; 

        assert(10 == base); /** @warning temporary measure **/

        if((NULL == s) || (MAX_RADIX < base))
                return NULL;

        len = strlen(s);

        n = bignum_create(0,len+1);

        if(NULL == n)
                return NULL;

        if('-' == s[0]){
                i++;
                n->isnegative = 1;
        } else if ('+' == s[0]){
                i++;
        }

        /** @todo convert all bases between 1 and 17 **/
        while('\0' != s[i]){
                if(!isdigit(s[i]))
                        return NULL;
                n->digits[len - i - 1] = (uint8_t)(s[i] - '0');
                n->lastdigit++;
                i++;
        }

        adjust_last(n);
        return n;
}

/** 
 *  @brief    Convert a bignum to a string ready for printing
 *  @param    n     Bignum to convert
 *  @param    base  Base to convert bignum to
 *  @return   char* Converted bignum
 */
char *bignum_bigtostr(bignum *n, unsigned int base){
        size_t allocate = 0, i = 0;
        char *s;
#if 0
        char *p, *q;
        bignum *b = bignum_create(base,2); /** @warning, why 2?**/
        bignum *z = bignum_create(0,1);
        bignum_div_t vs;
        bignum_div_t *v = &vs;
        v->quotient = bignum_create(0,1);
        v->remainder = bignum_create(0,1);
#endif
        
        if((MAX_RADIX < base) || (base <= 1))
                return NULL;
        if((NULL == n)) /* || (NULL == b) || (NULL == v->quotient) || (NULL == v->remainder)) */
                return NULL;

#if 0
        bignum_copy(v->quotient,n);
#endif

        /* +3 comes from '\0', possible +/- and array size +1*/
        allocate = n->lastdigit*((sizeof(n->digits[0])*8)/binlog(base))+3;

        s = calloc(allocate, sizeof(*s));
        if(NULL == s)
                return NULL;

        if(1 == n->isnegative)
                s[0] = '-';

        for(i = 0; i <= n->lastdigit; i++){
                size_t index = (n->lastdigit - i) + n->isnegative;
                s[index] = '0' + n->digits[i];
        } 

#if 0
        do{
                uint8_t rem;
                v = bignum_divide(v->quotient,b);
                rem = v->remainder->digits[0];
                s[i++] = "0123456789ABCDEF"[rem];
        } while(0 != bignum_compare(z,v->quotient));

        if(1 == n->isnegative)
                s[i++] = '-';

        q = s+i-1;
        p = s;
        while(p<q){
                char t;
                t = *p; *p = *q; *q = t;
                q--; p++;
        }
#endif

        return s;
}

/** 
 *  @brief    Convert an int to a bignum, allocating len amount of space
 *            for its digits
 *  @param    a         Integer value
 *  @return   bignum*   Bignum initialized to 'a'
 */
bignum *bignum_create(int initialize_to, size_t len){
        size_t x = 0;
        bignum *n = calloc(sizeof(*n), 1);

        if(NULL == n)
                return NULL;

      /*  assert(16 == INTERNAL_BASE);*/
        n->digits = calloc(len,sizeof(n->digits[0]));
        n->allocated = len;

        if(NULL == n->digits){
                free(n);
                return NULL;
        }

        memset(n->digits, 0, len);

        n->isnegative = 0 > initialize_to ? 1 : 0 ;  /* x = 0 > a */

        if(0 == initialize_to){
                n->lastdigit = 0;
                return n;
        }

        n->lastdigit = -1;
        x = (size_t)abs(initialize_to);

        do{
                n->lastdigit++;
                n->digits[n->lastdigit] = x % INTERNAL_BASE; 
                x = x / INTERNAL_BASE;
        } while(0 < x);

        return n;
}

/** 
 *  @brief    Destroy a bignum
 *  @param    n         Bignum to destroy, freeing memory in the process
 *  @return   void
 */
void bignum_destroy(bignum *n){
        if(NULL == n)
                return;
        free(n->digits);
        n->digits = NULL;
        free(n);
}

/** 
 *  @brief    Compare two bigums returning the result
 *  @param    a     Bignum 'a'
 *  @param    b     Bignum 'b'
 *  @return   int   1 == (a>b), 0 == (a==b), -1 == (a<b)
 */
int bignum_compare(bignum *a, bignum *b){
        size_t i = 0;

        assert((NULL != a) && (NULL != b));

        if((a->isnegative == 1) && (b->isnegative == 0))
                return -1;
        if((a->isnegative == 0) && (b->isnegative == 1))
                return 1;

        if(a->lastdigit < b->lastdigit)
                return a->isnegative ? 1 : -1;
        if(a->lastdigit > b->lastdigit)
                return a->isnegative ? -1 : 1;

        i = a->lastdigit + 1; 
        do{
                i--;
                if(a->digits[i] < b->digits[i])
                        return a->isnegative ? 1: -1;
                if(a->digits[i] > b->digits[i])
                        return a->isnegative ? -1: 1;
        } while(i > 0);

        return 0;
}

/** 
 *  @brief    Add two bignums together
 *  @param    result    The result of the addition
 *  @param    a         Operand 'a'
 *  @param    b         Operand 'b'
 *  @return   bignum*   Result of a+b
 */
bignum *bignum_add(bignum *a, bignum *b){
        uint16_t carry = 0;
        size_t i = 0, allocate;
        bignum *result;
        int isnegative;

        assert((NULL != a) && (NULL != b));

        if(a->isnegative == b->isnegative){
                isnegative = a->isnegative;
        } else {
                if(a->isnegative == 1){
                        a->isnegative = 0;
                        result = bignum_subtract(b,a);
                        a->isnegative = 1;
                        return result;
                } else if(b->isnegative == 1){
                        b->isnegative = 0;
                        result = bignum_subtract(a,b);
                        b->isnegative = 1;
                        return result;
                }
        }

        allocate = MAX(a->lastdigit , b->lastdigit) + 2;

        if(NULL == (result = bignum_create(0,allocate)))
                return NULL;

        result->isnegative = isnegative;

        result->lastdigit = MAX(a->lastdigit, b->lastdigit) + 1;

        for(i = 0; i <= result->lastdigit; i++){
                uint16_t j,ai,bi;
                ai = i > a->lastdigit ? 0 : a->digits[i];
                bi = i > b->lastdigit ? 0 : b->digits[i];
                j = carry + ai + bi;
                result->digits[i] = j % INTERNAL_BASE;
                carry = j / INTERNAL_BASE;
        }

        adjust_last(result);
        return result;
}

/** 
 *  @brief    Subtract two bignums, result = a - b
 *  @param    result    The result of the subtraction
 *  @param    a         Operand 'a'
 *  @param    b         Operand 'b'
 *  @return   bignum*   Result of a-b
 */
bignum *bignum_subtract(bignum *a, bignum *b){
        int borrow = 0;
        size_t i = 0;
        bignum *result;

        assert((NULL != a) && (NULL != b));

        if((1 == a->isnegative) || (1 == b->isnegative)){
                b->isnegative = b->isnegative == 1 ? 0 : 1;
                result = bignum_add(a,b);
                b->isnegative = b->isnegative == 1 ? 0 : 1;
                return result;
        }

        if(-1 == bignum_compare(a,b)){ 
                result = bignum_subtract(b,a);
                result->isnegative = 1;
                return result;
        }

        if(NULL == (result = bignum_create(0,a->lastdigit + 1)))
                return NULL;

        result->lastdigit = MAX(a->lastdigit, b->lastdigit);

        for(i = 0; i <= result->lastdigit; i++){
                int16_t x,ai,bi;
                
                ai = i > a->lastdigit ? 0 : a->digits[i];
                bi = i > b->lastdigit ? 0 : b->digits[i];

                x = ai - borrow - bi;

                borrow = 0 < a->digits[i] ? 0 : borrow;
                if(0 > x){
                        x += INTERNAL_BASE;
                        borrow = 1;
                }
                result->digits[i] = x % INTERNAL_BASE;
        }

        adjust_last(result);
        return result;
}

/** 
 *  @brief    Multiply two bignums together
 *  @param    result    The result of the multiplication
 *  @param    a         Operand 'a'
 *  @param    b         Operand 'b'
 *  @return   bignum*   Result of a*b
 */
bignum *bignum_multiply(bignum *a, bignum *b){
        bignum *row, *tmp, *result = NULL;
        size_t i,j,allocate;

        assert((NULL != a) && (NULL != b));

        allocate = (2 * MAX(a->lastdigit , b->lastdigit)) + 1;

        if(NULL == (result = bignum_create(0, allocate)))
                return NULL;

        if(NULL == (row = bignum_create(0, allocate)))
                return NULL;
        
        bignum_copy(row,a);

        for (i = 0; i <= b->lastdigit; i++){
                for(j = 0; j < b->digits[i]; j++){
                        tmp = result;
                        if(NULL == (result = bignum_add(result, row)))
                                return NULL;
                        bignum_destroy(tmp);
                }
                if(leftshift(row,1) < 0)
                        return NULL;
        }
        bignum_destroy(row);

        result->isnegative = a->isnegative ^ b->isnegative;

        adjust_last(result);
        return result;
}

/**
 *  @brief    Perform a copy of one bignum to another, taking care of
 *            the details of the structure
 *  @param    dst       Destination
 *  @param    src       Source
 *  @return   void
 */
void bignum_copy(bignum *dst, bignum *src){
        check((NULL != dst) && (NULL != src));

        if(NULL != dst->digits){
                free(dst->digits);
                dst->digits = NULL;
        }

        dst->digits = malloc(src->allocated * sizeof(dst->digits[0]));

        memmove(dst->digits,src->digits, sizeof(src->digits[0]) * (src->lastdigit + 1));

        dst->lastdigit = src->lastdigit;
        dst->allocated = src->allocated;
        dst->isnegative = src->isnegative;

        adjust_last(dst);
}

/** 
 *  @brief    Divide one bignum by another, producing a quotient and remainder
 *  @param    a         Operand 'a', dividend 
 *  @param    b         Operand 'b', divisor
 *  @return   bignum_div_t* Pointer to a structure containing the quotient and remainder
 */
bignum_div_t *bignum_divide(bignum *a, bignum *b){
        bignum_div_t *result;
        bignum *row, *tmp, *quotient, *remainder;
        size_t i;
        int asign, bsign;

        assert((NULL != a) && (NULL != b));

        if(NULL == (quotient = bignum_create(0,a->lastdigit + 1)))
                goto FAIL;
        if(NULL == (row = bignum_create(0,a->lastdigit + 1)))
                goto FREE_QUOTIENT;

        if(bignum_compare(b,quotient) == 0){/*quotient is zero at this point*/
                goto FREE_ROW;
        }

        asign = a->isnegative;
        bsign = b->isnegative;

        quotient->isnegative = a->isnegative ^ b->isnegative;

        a->isnegative = 0;
        b->isnegative = 0;

        quotient->lastdigit = a->lastdigit;

        i = a->lastdigit + 1;
        do{
                i--;
                if(leftshift(row,1) < 0)
                        return NULL;
                row->digits[0] = a->digits[i];
                quotient->digits[i] = 0;
                while(-1 < bignum_compare(row, b)){
                        quotient->digits[i]++;
                        tmp = bignum_subtract(row, b);
                        if(NULL == tmp)
                                goto FREE_ROW;
                        bignum_copy(row,tmp);
                        bignum_destroy(tmp);
                }
        } while (i > 0);
        remainder = row;

        a->isnegative = asign;
        b->isnegative = bsign;

        adjust_last(quotient);
        adjust_last(remainder);
        
        result = malloc(sizeof (*result));
        if(NULL == result)
                goto FREE_ROW;

        result->quotient = quotient;
        result->remainder = remainder;

        return result;

FREE_ROW:
        free(row);
FREE_QUOTIENT:
        free(quotient);
FAIL:
        return NULL;
}

/**** Functions with internal linkage ****************************************/

/** 
 *  @brief    Cleanup a bignum after an operation
 *  @param    n         Operand 'n' 
 *  @return   void
 */
static void adjust_last(bignum * n)
{
        assert(NULL != n);
        while ((n->lastdigit > 0) && (n->digits[n->lastdigit] == 0))
                n->lastdigit--;

        if ((n->lastdigit == 0) && (n->digits[0] == 0))
                n->isnegative = 0;
}

/** 
 *  @brief    Multiply a bignum by radix^d
 *  @param    n         Operand 'n' 
 *  @param    d         Operand 'n' 
 *  @return   int       0 == Ok, 0 > bad  ;
 */
static int leftshift(bignum *n, unsigned int d){
        size_t i;
        uint8_t *p;

        assert(NULL != n);

        if((0 == n->lastdigit) && (0 == n->digits[0]))
                return 0;

        if(NULL == (p = realloc(n->digits,n->allocated+d)))
                return -1;

        n->digits = p;
        n->allocated += d;

        i = n->lastdigit + 1;
        do{
                i--;
                n->digits[i + d] = n->digits[i];
        } while(i > 0);

        for(i = 0; i < d; i++)
                n->digits[i] = 0;

        n->lastdigit = n->lastdigit + d;

        return 0;
}

/** 
 *  @brief    Calculate the binary logarithm, for more efficient examples see
 *            http://graphics.stanford.edu/~seander/bithacks.html or
 *            "Bit Twiddling Hacks by Sean Eron Anderson"
 *  @param    v         Value to calculate the binary logarithm of
 *  @return   uint8_t   Binary log
 */
static uint8_t binlog(size_t v){
        uint8_t r = 0;
        while(v >>= 1)
                r++;
        return r;
}

/** 
 *  @brief    Performs roughly the same job as "assert" but will
 *            not get defined out by NDEBUG. This should be wrapped
 *            in a macro however so you do not have to type __FILE__
 *            and __LINE__ out repeatedly 
 *  @param    n         Operand 'n' 
 *  @param    d         Operand 'n' 
 *  @return   void
 */
static void _check(int checkme, char *file, int line){
        if(0 == checkme){
                fprintf(stderr,"check failed in %s on line %d.\n", file, line);
                abort();
        }
        return;
}

