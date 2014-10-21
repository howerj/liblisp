
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <limits.h>
#include <stdbool.h>
#include "bignum.h"

#define MY_RADIX        (10)

static void _check(int checkme, char *file, int line);
#define check(X) _check((X),__FILE__,__LINE__)

void test_compare(char *a, char *b){
        char *sa,*sb;
        int tmp;
        bignum *na,*nb;

        check(NULL != (na = bignum_strtobig(a,MY_RADIX)));
        check(NULL != (nb = bignum_strtobig(b,MY_RADIX)));
        tmp = bignum_compare(na,nb);
        check(NULL != (sa = bignum_bigtostr(na,MY_RADIX)));
        check(NULL != (sb = bignum_bigtostr(nb,MY_RADIX)));
        printf("%s\t<=>\t%s\t=\t%d\n",sa,sb,tmp);

        bignum_destroy(na);
        bignum_destroy(nb);
        free(sa);
        free(sb);
        return;
}

void 
perform_test(bignum *(*ftest)(bignum *a, bignum *b), char *a, char *b, char *test){
        char *sap, *sbp, *sresult;
        bignum *na,*nb,*nresult;
        
        check(NULL != (na = bignum_strtobig(a,MY_RADIX)));
        check(NULL != (nb = bignum_strtobig(b,MY_RADIX)));
        check(NULL != (nresult = (ftest)(na,nb)));

        /*check we have not messed up the arguments by accident*/
        check(NULL != (sap = bignum_bigtostr(na,MY_RADIX)));
        check(NULL != (sbp = bignum_bigtostr(nb,MY_RADIX)));
        check(NULL != (sresult = bignum_bigtostr(nresult,MY_RADIX)));

        printf("%s\t%s\t%s\t=\t%s\n",a,test,b,sresult);

        bignum_destroy(na);
        bignum_destroy(nb);
        bignum_destroy(nresult);
        free(sap);
        free(sbp);
        free(sresult);
        return;
}

void 
perform_division(char *a, char *b){
        char *squotient, *sremainder;
        bignum *na,*nb;
        bignum_div_t *result;
        
        check(NULL != (na = bignum_strtobig(a,MY_RADIX)));
        check(NULL != (nb = bignum_strtobig(b,MY_RADIX)));

        /* we want to check that out divide can handle a divisor of zero */
        if(NULL == (result = bignum_divide(na,nb))){
                printf("Error on input; possible division by zero?\n");
                return;
        }

        check(NULL != (squotient = bignum_bigtostr(result->quotient,MY_RADIX)));
        check(NULL != (sremainder = bignum_bigtostr(result->remainder,MY_RADIX)));
        printf("%s\t/\t%s\t=\t%s\n",a,b,squotient);
        printf("%s\t%%\t%s\t=\t%s\n",a,b,sremainder);
        bignum_destroy(na);
        bignum_destroy(nb);
        bignum_destroy(result->quotient);
        bignum_destroy(result->remainder);
        free(result);
        free(squotient);
        free(sremainder);
        return;
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

int main(int argc, char *argv[]){
        char *a,*b;

        if(argc != 3){
                fprintf(stderr,"usage: %s bignum bignum\n", argv[0]);
                return EXIT_FAILURE;
        }

        a = argv[1];
        b = argv[2];

        test_compare(a,b);
        perform_test(bignum_add,a,b,"+");
        perform_test(bignum_subtract,a,b,"-");
        perform_test(bignum_multiply,a,b,"*");
        perform_division(a,b);

        return EXIT_SUCCESS;
}
