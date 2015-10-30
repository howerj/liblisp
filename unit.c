/** @file     unit.c
 *  @brief    unit tests for liblisp interpreter public interface
 *  @author   Richard Howe (2015)
 *  @license  LGPL v2.1 or Later 
 *            <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email    howe.r.j.89@gmail.com
 *
 *  @todo     Each (major) function in each file should have tests written
 *            for it, this does (might) not include accessor functions like 
 *            "get_int"
 *  @todo     It is possible to test if a function throws an
 *            assertion when passed invalid data.
 *  @note     This could be moved to "util.c" so it can be reused.
 *  @note     Other functionality could include generating a random test
 *            vector, logging and more!
 *  @note     While it is not imperative that each test has the memory
 *            it uses released, it is desired.
 *  @note     All functions under test should have assertions turned on
 *            when they were compiled. This test suite can handle SIGABRT
 *            signals being generated, it will fail the unit that caused
 *            it and continue processing the next tests. 
 **/

/*** module to test ***/
#include "liblisp.h"
/**********************/

#include <setjmp.h>
#include <signal.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/*** very minimal test framework ***/

static unsigned passed, failed;
static double timer;
static clock_t start_time, end_time;
static time_t rawtime;

int color_on = 0, jmpbuf_active = 0;
jmp_buf current_test;
unsigned current_line = 0;
const char *current_expr;

static const char *reset(void)  { return color_on ? "\x1b[0m"  : ""; }
static const char *red(void)    { return color_on ? "\x1b[31m" : ""; }
static const char *green(void)  { return color_on ? "\x1b[32m" : ""; }
static const char *yellow(void) { return color_on ? "\x1b[33m" : ""; }
static const char *blue(void)   { return color_on ? "\x1b[34m" : ""; }

static void unit_tester(int test, const char *msg, unsigned line) {
        if(test) passed++, printf("      %sok%s:\t%s\n", green(), reset(), msg); 
        else     failed++, printf("  %sFAILED%s:\t%s (line %d)\n", red(), reset(), msg, line);
}

static void print_statement(char *stmt) {
        printf("   %sstate%s:\t%s\n", blue(), reset(), stmt);
}

static void print_note(char *name) { printf("%s%s%s\n", yellow(), name, reset()); }

#define MAX_SIGNALS (256)
static char *(sig_lookup[]) = { /*List of C89 signals and their names*/
        [SIGABRT]       = "SIGABRT",
        [SIGFPE]        = "SIGFPE",
        [SIGILL]        = "SIGILL",
        [SIGINT]        = "SIGINT",
        [SIGSEGV]       = "SIGSEGV",
        [SIGTERM]       = "SIGTERM",
        [MAX_SIGNALS]   = NULL
};

static void sig_abrt_handler(int sig) {
        char *sig_name = "UNKNOWN SIGNAL";
        if((sig > 0) && (sig < MAX_SIGNALS) && sig_lookup[sig])
                sig_name = sig_lookup[sig];
        printf("Caught %s (signal number %d)\n", sig_name, sig);
        if(jmpbuf_active) {
                jmpbuf_active = 0;
                longjmp(current_test, 1);
        }
}

/**@brief Advance the test suite by testing and executing an expression. This
 *        framework can catch assertions that have failed within the expression
 *        being tested.
 * @param EXPR The expression should yield non zero on success **/
#define test(EXPR)\
        do {\
                current_line = __LINE__;\
                current_expr = #EXPR;\
                signal(SIGABRT, sig_abrt_handler);\
                if(!setjmp(current_test)) {\
                        jmpbuf_active = 1;\
                        unit_tester( ((EXPR) != 0), current_expr, current_line);\
                } else {\
                        unit_tester(0, current_expr, current_line);\
                        signal(SIGABRT, sig_abrt_handler);\
                }\
                signal(SIGABRT, SIG_DFL);\
                jmpbuf_active = 0;\
        } while(0);

/**@brief print out and execute a statement that is needed to further a test
 * @param STMT A statement to print out (stringify first) and then execute**/
#define state(STMT) do{ print_statement( #STMT ); STMT; } while(0);

/**@brief As signals are caught (such as those generated by abort()), we exit
 *        the unit test function by returning from it instead. */
#define return_if(EXPR) if((EXPR)) { printf("unit test framework failed on line '%d'\n", __LINE__); return -1;}

static int sstrcmp(const char *s1, const char *s2) {
        if(!s1 || !s2) return -256;
        return strcmp(s1, s2);
}

static int unit_test_start(const char *unit_name) {
        time(&rawtime);
        if(signal(SIGABRT, sig_abrt_handler) == SIG_ERR) {
                printf("signal handler installation failed");
                return -1;
        }
        start_time = clock();
        printf("%s unit tests\n%sbegin:\n\n", unit_name, asctime(localtime(&rawtime)));
        return 0;
}

static unsigned unit_test_end(const char *unit_name) {
        end_time = clock();
        timer = ((double) (end_time - start_time)) / CLOCKS_PER_SEC;
        printf("\n\n%s unit tests\npassed  %u/%u\ntime    %fs\n", unit_name, passed, passed+failed, timer);
        return failed;
}

/*** end minimal test framework ***/

int main(int argc, char **argv) {
        if(argc > 1)
                while(++argv, --argc) {
                        if(!strcmp("-c", argv[0])) {
                                color_on = 1;
                        } else if (!strcmp("-h", argv[0])) {
                                printf("liblisp unit tests\n\tusage ./%s (-c)? (-h)?\n", argv[0]);
                        } else {
                                printf("unknown argument '%s'\n", argv[0]);
                        }
                }

        unit_test_start("liblisp");

        /** @todo tr.c, io.c, valid.c, read.c, print.c, eval.c,
         *        compile.c, subr.c, ... */
        { 
                print_note("util.c");

                test(ilog2(0)   == INT32_MIN);
                test(ilog2(1)   == 0);
                test(ilog2(2)   == 1);
                test(ilog2(5)   == 2);
                test(ilog2(255) == 7);
                test(ilog2(256) == 8);
                test(ilog2(UINT64_MAX) == 63);

                test(ipow(0, 0) == 1);
                test(ipow(0, 1) == 0);
                test(ipow(3, 3) == 27);
                test(ipow(3, 4) == 81);
                test(ipow(2, 25) == (1 << 25));
                test(ipow(24, 2) == 576);

                test(is_number("0xfAb"));
                test(is_number("-01234567"));
                test(is_number("+1000000000000000000000000000003"));
                test(!is_number(""));
                test(!is_number("+"));
                test(!is_number("-"));

                test(balance("(((") == 3);
                test(balance("))") == -2);
                test(balance("") == 0);
                test(balance("\"(") == 0);
                test(balance("( \"))))(()()()(()\\\"())\")") == 0);
                test(balance("(a (b) c (d (e (f) \")\" g)))") == 0);
                test(balance("((a b) c") == 1);

                bitfield *b = NULL;
                state(b = bit_new(1024));
                return_if(!b);

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

                char *s = NULL;
                state(s = vstrcatsep(",", "a", "b", "c", "", "foo", "bar", NULL));
                test(!sstrcmp("a,b,c,,foo,bar", s));
                free(s);

                /* @todo regex_match, djb2, lstrcatend, xorshift128plus, knuth*/ 
        }

        { /* hash.c hash table tests */
                hashtable *h = NULL;
                print_note("hash.c");
                state(h = hash_create(64));
                return_if(!h);
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

                print_note("lisp.c");
                /*while unit testing eschews state being held across tests it is makes
                 *little sense in this case*/
                state(l = lisp_init()); 
                test(!lisp_set_logging(l, io_nout()));
                return_if(!l);
                test(!lisp_eval_string(l, ""));
                test(get_int(lisp_eval_string(l, "(+ 2 2)")) == 4);
                test(get_int(lisp_eval_string(l, "(* 3 2)")) == 6);

                cell *x = NULL, *y = NULL, *z = NULL;

                state(x = intern(l, lstrdup("foo")));
                state(y = intern(l, lstrdup("foo")));
                state(z = intern(l, lstrdup("bar")));
                test(x == y && x != NULL);
                test(x != z);

                test(gsym_error() == lisp_eval_string(l, "(> 'a 1)"));
                test(is_sym(x));
                test(is_asciiz(x));
                test(!is_str(x));
                test(gsym_error() == lisp_eval_string(l, "(eval (cons quote 0))"));

                state(lisp_destroy(l));
        }
        return unit_test_end("liblisp");; /*should be zero!*/
}

