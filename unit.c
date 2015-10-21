/** @file     unit.c
 *  @brief    unit tests for liblisp interpreter public interface
 *  @author   Richard Howe (2015)
 *  @license  LGPL v2.1 or Later 
 *            <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email    howe.r.j.89@gmail.com
 *
 *  @todo     Each (major) function in each file should have tests written
 *            for it, this does not include accessor functions like "get_int"
 *  @note     This could be moved to "util.c" so it can be reused.
 *  @note     Other functionality could include generating a random test
 *            vector, optional colorizing, logging and more!
 **/

/*** module to test ***/
#include "liblisp.h"
/**********************/

#ifdef NDEBUG 
#undef NDEBUG /*@warning do not remove this*/
#endif

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <time.h>

/*** very minimal test framework ***/

static unsigned passed, failed;
static int color = 0;

static char *reset(void)  { return color ? "\x1b[0m"  : ""; }
static char *red(void)    { return color ? "\x1b[31m" : ""; }
static char *green(void)  { return color ? "\x1b[32m" : ""; }
static char *yellow(void) { return color ? "\x1b[33m" : ""; }
static char *blue(void)   { return color ? "\x1b[34m" : ""; }
static void pass(void)    { passed++; }
static void fail(void)    { failed++; }
static void print_note(char *name) { printf("%s%s%s\n", yellow(), name, reset()); }

/**@brief Advance the test suite by testing and executing an expression. 
 * @param EXPR The expression should yield non zero on success **/
#define test(EXPR) do {\
        if(!(EXPR)) printf("  %sFAILED%s:\t" #EXPR " (line '%d')\n", red(), reset(), __LINE__), fail();\
        else        printf("      %sok%s:\t" #EXPR "\n", green(), reset()), pass();\
        } while(0);

/**@brief print out and execute a statement that is needed to further a test
 * @param STMT A statement to print out (stringify first) and then execute**/
#define state(STMT) do{\
                    printf("   %sstate%s:\t" #STMT "\n", blue(), reset());\
                    STMT;\
        } while(0);

/*** end minimal test framework ***/

static int sstrcmp(char *s1, char *s2) {
        if(!s1 || !s2) return -1;
        return strcmp(s1, s2);
}

int main(int argc, char **argv) {
        double timer;
        clock_t start, end;
        time_t rawtime;
        time(&rawtime);

        if(argc > 1)
                while(++argv,--argc)
                        if(!strcmp("-c", argv[0])) {
                                color = 1;
                        } else if (!strcmp("-h", argv[0])) {
                                printf("liblisp unit tests\n\tusage ./%s (-c)? (-h)?\n", argv[0]);
                        } else {
                                fprintf(stderr, "unknown argument '%s'\n", argv[0]);
                        }

        printf("liblisp unit tests\n%sbegin:\n\n", asctime(localtime(&rawtime)));

        start = clock();
        /** @todo tr.c, io.c, valid.c, read.c, print.c, eval.c,
         *        compile.c, subr.c, ... */
        { 
                bitfield *b;
                char *s;
                print_note("util.c");

                test(ilog2(0)   == INT32_MIN);
                test(ilog2(1)   == 0);
                test(ilog2(2)   == 1);
                test(ilog2(5)   == 2);
                test(ilog2(255) == 7);
                test(ilog2(256) == 8);
                test(ilog2(UINT64_MAX) == 63);

                test(is_number("0xfAb"));
                test(is_number("-01234567"));
                test(is_number("+1000000000000000000000000000003"));
                test(!is_number(""));
                test(!is_number("+"));
                test(!is_number("-"));

                test(balance("(((") == 3);
                test(balance("))") == -2);
                test(balance("") == 0);
                test(balance("( \"))))(()()()(()\\\"())\")") == 0);
                test(balance("(a (b) c (d (e (f) \")\" g)))") == 0);
                test(balance("((a b) c") == 1);

                test(b = bit_new(1024));

                state(bit_set(b, 1023));
                state(bit_set(b, 37));
                state(bit_toggle(b, 37));
                state(bit_set(b, 0));
                state(bit_unset(b, 0));

                test(bit_get(b, 1023));
                test(!bit_get(b, 37));
                test(!bit_get(b, 0));

                bit_delete(b);

                test(!is_fnumber(""));
                test(is_fnumber("+0."));
                test(is_fnumber("123")); /*this passes, see header*/
                test(is_fnumber("1e-3"));
                test(is_fnumber("1.003e+34"));
                test(is_fnumber("1e34"));
                test(is_fnumber("93.04"));

                test(match("",""));
                test(match("abc","abc"));
                test(!match("abC","abc"));
                test(match("aaa*","aaaXX"));
                test(!match("aaa*","XXaaaXX"));
                test(match(".bc","abc"));
                test(match("a.c","aXc"));
                test(!sstrcmp("a,b,c,,foo,bar", s = vstrcatsep(",", "a", "b", "c", "", "foo", "bar", NULL)));
                free(s);
                /* @todo lstrdup, regex_match, djb2, lstrcatend,
                 * ipow, xorshift128plus
                 */ 
        }

        { /* hash.c hash table tests */
                hashtable *h;
                print_note("hash.c");
                state(h = hash_create(64));
                assert(h);
                test(!hash_insert(h, "key1", "val1"));
                test(!hash_insert(h, "key2", "val2"));
                test(!hash_insert(h, "heliotropes", "val3")); 
                test(!hash_insert(h, "neurospora",  "val4"));
                test(!hash_insert(h, "depravement", "val5"));
                test(!hash_insert(h, "serafins",    "val6"));
                test(!hash_insert(h, "playwright",  "val7"));
                test(!hash_insert(h, "snush",       "val8"));
                test(!hash_insert(h, "",            "val9"));
                test(!hash_insert(h, "nil",         ""));
                test(!hash_insert(h, "a",           "x"));
                test(!hash_insert(h, "a",           "y"));
                test(!hash_insert(h, "a",           "z"));
                test(!sstrcmp("val1", hash_lookup(h, "key1")));
                test(!sstrcmp("val2", hash_lookup(h, "key2")));
                test(!sstrcmp("val3", hash_lookup(h, "heliotropes")));
                test(!sstrcmp("val4", hash_lookup(h, "neurospora")));
                test(!sstrcmp("val5", hash_lookup(h, "depravement")));
                test(!sstrcmp("val6", hash_lookup(h, "serafins")));
                test(!sstrcmp("val7", hash_lookup(h, "playwright")));
                test(!sstrcmp("val8", hash_lookup(h, "snush")));
                test(!sstrcmp("val9", hash_lookup(h, "")));
                test(!sstrcmp("",     hash_lookup(h, "nil")));
                test(!sstrcmp("z",    hash_lookup(h, "a")));
                state(hash_destroy(h));
        }

        { /* lisp.c (and the lisp interpreter in general) */
                lisp *l;
                cell *x;

                print_note("lisp.c");
                /*while unit testing eschews state being held across tests it is makes
                 *little sense in this case*/
                state(l = lisp_init()); 
                assert(l);
                test(get_int(lisp_eval_string(l, "(+ 2 2)")) == 4);
                test(get_int(lisp_eval_string(l, "(* 3 2)")) == 6);
                test(!strcmp(get_str(lisp_eval_string(l, "(join \" \" \"Hello\" \"World!\")")), "Hello World!"));
                test(x =  intern(l, lstrdup("foo")));
                test(x == intern(l, lstrdup("foo")));
                test(x != intern(l, lstrdup("bar")));
                test(is_sym(x));
                test(is_asciiz(x));
                test(!is_str(x));

                state(lisp_destroy(l));
        }
        end = clock();

        timer = ((double) (end - start)) / CLOCKS_PER_SEC;
        printf("\npassed  %u/%u\ntime    %fs\n", passed, passed+failed, timer);
        return failed; /*should be zero!*/
}

