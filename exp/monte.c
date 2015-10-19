/* use monte-carlo methods to approximate the value of pi
 * this is to be used as a bench mark against which the
 * lisp interpreters version of this.  */
#include <stdio.h>
#include <stdlib.h>

#define PI  (3.141592f)
#define ABS ((X) < 0 ? -(X) : (X))

double frandom(void) { return (double)rand() / RAND_MAX; }
double sum_of_squares(double x, double y) { return (x * x) + (y * y); }

double monte_carlo_pi(long iterations) {
        long count = 0, i = iterations;
        while(iterations--)
                if(sum_of_squares(frandom(), frandom()) <= 1.) 
                        count++;
        return ((double)count / i) * 4.;
}

int main(int argc, char **argv) {
        double mpi;
        char *e = NULL;
        long i = 0;
        if(argc != 2)
                return fprintf(stderr, "usage: %s iterations\n", argv[0]), -1;
        i = strtol(argv[1], &e, 10);
        if(i <= 0 || *e)
                return fprintf(stderr, "'%s' is not a strictly positive integer\n", argv[1]), -1;
        srand(0);
        mpi = monte_carlo_pi(i);
        printf("calculated: %f\nactual: %f\nerror: %f\n", mpi, PI, PI - mpi);
        return 0;
}

