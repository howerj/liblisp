/**
 * TODO:
 *      * Simple optimization of ignoring beginning and end of text
 *      * Turn into library
 * See:
 * <https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Longest_common_subsequence>
 * <http://www.algorithmist.com/index.php/Longest_Common_Subsequence>
 * <https://en.wikipedia.org/wiki/Longest_common_subsequence_problem>
 *
 * XXX BUGS:
 *      * If one file is empty it will not work, instead it returns null
 *      * Not valgrind clean
 *      * unsigned array wrap around
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include "diff.h"

#define MAX(X, Y) ((X) > (Y) ? (X) : (Y))

/*
function LCS(X[1..m], Y[1..n])
    C = array(0..m, 0..n)
    for i := 0..m
       C[i,0] = 0
    for j := 0..n
       C[0,j] = 0
    for i := 1..m
        for j := 1..n
            if X[i] = Y[j]
                C[i,j] := C[i-1,j-1] + 1
            else
                C[i,j] := max(C[i,j-1], C[i-1,j])
    return C*/

diff *lcs(char *x[], size_t xlen, char *y[], size_t ylen)
{
        diff *d;
        unsigned *c;
        size_t m=xlen, n=ylen, i, j;
        if(!x || !y)
                return NULL;
        if(!(c = calloc((m+1)*(n+1),sizeof(*c))))
                return perror("calloc failed"), NULL;
        if(!(d = calloc(1, sizeof(*d))))
                return perror("calloc failed"), NULL;
        for(i = 1; i <= m; i++)
                for(j = 1; j < n; j++) /*XXX This is not "<="? What did I do?*/
                        if (!strcmp(x[i-1], y[j-1]))
                                c[i*n+j] = c[(i-1)*n+(j-1)] + 1;
                        else
                                c[i*n+j] = MAX(c[i*n+(j-1)], c[(i-1)*n+j]);
        d->c = c;
        d->m = m;
        d->n = n;
        return d;
}

/* function printDiff(C[0..m,0..n], X[1..m], Y[1..n], i, j)
    if i > 0 and j > 0 and X[i] = Y[j]
        printDiff(C, X, Y, i-1, j-1)
        print "  " + X[i]
    else if j > 0 and (i = 0 or C[i,j-1] ≥ C[i-1,j])
        printDiff(C, X, Y, i, j-1)
        print "+ " + Y[j]
    else if i > 0 and (j = 0 or C[i,j-1] < C[i-1,j])
        printDiff(C, X, Y, i-1, j)
        print "- " + X[i]
    else
        print "" */

void print_diff_inner(diff *d, char *x[], char *y[], size_t i, size_t j)
{
        if (i > 0 && j > 0 && !strcmp(x[i-1], y[j-1])) {
                print_diff_inner(d, x, y, i-1, j-1);
                printf("  %s", x[i-1]);
        } else if (j > 0 && (i == 0 || d->c[(i*(d->n))+(j-1)] >= d->c[((i-1)*(d->n))+j])) {
                print_diff_inner(d, x, y, i, j-1);
                printf("+ %s", y[j-1]);
        } else if(i > 0 && (j == 0 || d->c[(i*(d->n))+(j-1)] < d->c[((i-1)*(d->n))+j])) {
                print_diff_inner(d, x, y, i-1, j);
                printf("- %s", x[i-1]);
        }
}

void print_diff(diff *d, char **x, char **y)
{
        assert(d && x && y);
        print_diff_inner(d, x, y, d->m, d->n);
}

char **fill_array_with_lines(FILE *f, size_t *returned_length)
{
        char *line = NULL, **s = NULL;
        size_t ignore = 0, nl = 1;
        while(getline(&line, &ignore, f) > 0) {
                if(!(s = realloc(s,(nl+1)*sizeof(*s))))
                        return NULL;
                s[nl-1] = line;
                s[nl] = NULL;
                nl++;
                line = NULL;
                ignore = 0;
        }
        *returned_length = nl - 1;
        free(line);
        return s;
}

#if 0
int main(int argc, char **argv) 
{
        diff *d;
        char **a, **b, *m;
        size_t la, lb; /*lengths*/
        FILE *fa, *fb;

        if(argc < 3)
                return fprintf(stderr, "usage: %s file file\n", argv[0]);

        fa = fopen_or_fail(argv[1], "r");
        fb = fopen_or_fail(argv[2], "r");

        a = fill_array_with_lines(fa, &la);
        b = fill_array_with_lines(fb, &lb);

        if(!(d = lcs(a, la, b, lb)))
                return 0; /*empty files*/
        print_diff(d, a, b);

        while((m=*(a++))) free(m);
        while((m=*(b++))) free(m);

        free(d->c);
        free(d);
        fclose(fa);
        fclose(fb);
        return 0;
}
#endif
