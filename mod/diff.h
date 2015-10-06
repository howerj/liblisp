/** @file       diff.h
 *  @brief      diff module interface
 *  @email      howe.r.j.89@gmail.com**/

#include <stddef.h>

typedef struct {
        unsigned *c;    /*2D array of Longest Common Substrings*/
        size_t m, n;    /*2D array dimensions*/
} diff;

diff *lcs(char *x[], size_t xlen, char *y[], size_t ylen);


