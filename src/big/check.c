#include <stdio.h>  /* fprintf */
#include <stdlib.h> /* abort */
#include "check.h"

/** 
 *  @brief    Performs roughly the same job as "assert" but will
 *            not get defined out by NDEBUG. This should be wrapped
 *            in a macro however so you do not have to type __FILE__
 *            and __LINE__ out repeatedly 
 *  @param    n         Operand 'n' 
 *  @param    d         Operand 'n' 
 *  @return   void
 */
void _check(int checkme, char *file, int line){
        if(0 == checkme){
                fprintf(stderr,"check failed in %s on line %d.\n", file, line);
                abort();
        }
        return;
}

