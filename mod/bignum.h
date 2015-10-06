
#include <stddef.h>
#include <stdint.h>
#define MAX_RADIX       (16)

typedef struct bignum bignum;
typedef struct {
        bignum *quotient;
        bignum *remainder;
} bignum_div_t;

/** @brief    Convert a string to a bignum
 *  @param    s         String that needs converting
 *  @param    base      Base to convert string from
 *  @return   bignum*   Initialized bignum from string**/
bignum *bignum_strtobig(const char *s, unsigned int base);

/** @brief    Convert a bignum to a string ready for printing
 *  @param    n     Bignum to convert
 *  @param    base  Base to convert bignum to
 *  @return   char* Converted bignum **/
char *bignum_bigtostr(bignum *n, unsigned int base);

/** @brief    Convert an int to a bignum, allocating len amount of space
 *            for its digits
 *  @param    a         Integer value
 *  @param    len       Size of initial bignum
 *  @return   bignum*   Bignum initialized to 'a' **/
bignum *bignum_create(int initialize_to, size_t len);

/** @brief    Destroy a bignum
 *  @param    n         Bignum to destroy, freeing memory in the process **/
void bignum_destroy(bignum *n);

/** @brief    Compare two bigums returning the result
 *  @param    a     Bignum 'a'
 *  @param    b     Bignum 'b'
 *  @return   int   1 == (a>b), 0 == (a==b), -1 == (a<b) **/
int bignum_compare(bignum *a, bignum *b);

/** @brief    Add two bignums together
 *  @param    result    The result of the addition
 *  @param    a         Operand 'a'
 *  @param    b         Operand 'b'
 *  @return   bignum*   Result of a+b **/
bignum *bignum_add(bignum *a, bignum *b);

/** @brief    Subtract two bignums, result = a - b
 *  @param    result    The result of the subtraction
 *  @param    a         Operand 'a'
 *  @param    b         Operand 'b'
 *  @return   bignum*   Result of a-b **/
bignum *bignum_subtract(bignum *a, bignum *b);

/** @brief    Multiply two bignums together
 *  @param    result    The result of the multiplication
 *  @param    a         Operand 'a'
 *  @param    b         Operand 'b'
 *  @return   bignum*   Result of a*b **/
bignum *bignum_multiply(bignum *a, bignum *b);

/** @brief    Divide one bignum by another, producing a quotient and remainder
 *  @param    a         Operand 'a', dividend 
 *  @param    b         Operand 'b', divisor
 *  @return   bignum_div_t* a structure containing the quotient and remainder**/
bignum_div_t* bignum_divide(bignum *a, bignum *b);

/** @brief    Perform a copy of one bignum to another, taking care of
 *            the details of the structure
 *  @param    dst       Destination
 *  @param    src       Source**/
void bignum_copy(bignum *dst, bignum *src);

