/*Testing running multiple instances of the interpreter. This could very much
 *be improved. It is a pretty lame test at the moment.*/
#include <pthread.h>
#include <liblisp.h>
#include <stdlib.h>

pthread_mutex_t mutex_write = PTHREAD_MUTEX_INITIALIZER;

void *lisp_thead_eval(void *ptr) {
        lisp *l;
        cell *x;
        if(!(l = lisp_init())) return NULL;
        x = lisp_eval_string(l, (char*) ptr);
        pthread_mutex_lock(&mutex_write);
        lisp_print(l, x);
        pthread_mutex_unlock(&mutex_write);
        lisp_destroy(l);
        return NULL;
}

int main(void)
{
        pthread_t thread_1, thread_2;
        int ret1, ret2, i;
        for(i = 0; i < 10; i++) {
                ret1 = pthread_create(&thread_1, NULL, lisp_thead_eval, 
                                (void*)"(+ 2 2) ");
                if(ret1) {
                        fprintf(stderr, "thread 1 returned %d\n", ret1);
                        exit(1);
                }

                ret2 = pthread_create(&thread_2, NULL, lisp_thead_eval, 
                                (void*)"(put \"Another thread\n\") ");
                if(ret2) {
                        fprintf(stderr, "thread 2 returned %d\n", ret1);
                        exit(1);
                }

                pthread_join(thread_1, NULL);
                pthread_join(thread_2, NULL);
        }
        return 0;
}
