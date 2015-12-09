/** @file     unit.c
 *  @brief    unit tests for liblisp interpreter public interface
 *  @author   Richard Howe (2015)
 *  @license  LGPL v2.1 or Later 
 *            <https://www.gnu.org/licenses/old-licenses/lgpl-2.1.en.html> 
 *  @email    howe.r.j.89@gmail.com
 *
 *  @todo     It is possible to test if a function throws an
 *            assertion when passed invalid data. If it does not throw
 *            an assertion it might irrevocably corrupt the program
 *            state.
 *  @todo     This unit test framework should aim for 100% coverage in
 *            each file, however difficult it might be, Gcov can be
 *            used to get the current coverage percentage:
 *            (see <https://gcc.gnu.org/onlinedocs/gcc/Gcov.html>).
 *  @note     This could be moved to "util.c" so it can be reused, the
 *            unit test functionality has been encapsulated in a series
 *            of functions here that should be quite module, if limited
 *            to single threaded applications.
 *  @note     All functions under test should have assertions turned on
 *            when they were compiled. This test suite can handle SIGABRT
 *            signals being generated, it will fail the unit that caused
 *            it and continue processing the next tests. 
 **/

/*** module to test ***/
#include "liblisp.h"
/**********************/

#include <assert.h>
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

static int caught_signal;
static void print_caught_signal_name(void) {
        char *sig_name = "UNKNOWN SIGNAL";
        if((caught_signal > 0) && (caught_signal < MAX_SIGNALS) && sig_lookup[caught_signal])
                sig_name = sig_lookup[caught_signal];
        printf("Caught %s (signal number %d)\n", sig_name, caught_signal);\
}

static void sig_abrt_handler(int sig) {
        caught_signal = sig;
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
                        print_caught_signal_name();\
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
                        if(!strcmp("-c", argv[0]))
                                color_on = 1;
                        else if (!strcmp("-h", argv[0]))
                                printf("liblisp unit tests\n\tusage ./%s (-c)? (-h)?\n", argv[0]);
                        else
                                printf("unknown argument '%s'\n", argv[0]);
                }

        unit_test_start("liblisp");
        { 
                print_note("util.c");
                /**@todo lltostr*/

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
                test(!is_fnumber("1e"));
                test(!is_fnumber("-1e"));
                test(!is_fnumber("1-e"));
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
                test(!match("a\\.c","aXc"));
                test(match("a\\.c","a.c"));

                char *s = NULL;
                state(s = vstrcatsep(",", "a", "b", "c", "", "foo", "bar", NULL));
                test(!sstrcmp("a,b,c,,foo,bar", s));
                free(s);

                char *t = NULL, *s1 = "Hello,", *s2 = " World!";
                state(t = calloc(16,1));
                state(strcpy(t, s1));
                test(((size_t)(lstrcatend(t, s2) - t)) == (strlen(s1) + strlen(s2)));
                free(t);

                /*test tr, or translate, functionality*/
                size_t trinsz = 0;
                uint8_t trout[128] = {0}, *trin = (uint8_t*)"aaabbbcdaacccdeeefxxxa";
                tr_state *tr1;
                state(tr1 = tr_new());
                state(trinsz = strlen((char*)trin));
                test(tr_init(tr1, "", (uint8_t*)"abc", (uint8_t*)"def") == TR_OK);
                test(tr_block(tr1, trin, trout, trinsz) == trinsz);
                test(!strcmp((char*)trout, "dddeeefdddfffdeeefxxxd"));
                test(tr_init(tr1, "s", (uint8_t*)"abc", (uint8_t*)"def") == TR_OK);
                state(memset(trout, 0, 128));
                test(tr_block(tr1, trin, trout, trinsz) <= trinsz);
                test(!strcmp((char*)trout, "defddfdeeefxxxd"));
                state(tr_delete(tr1));

                /*know collisions for the djb2 hash algorithm*/
                test(djb2("heliotropes", strlen("heliotropes")) == djb2("neurospora", strlen("neurospora")));
                test(djb2("depravement", strlen("depravement")) == djb2("serafins", strlen("serafins")));
                /*should not collide*/
                test(djb2("heliotropes", strlen("heliotropes")) != djb2("serafins", strlen("serafins")));

                /* @todo regex_match, xorshift128plus, knuth*/ 
        }

        { /*io.c test; @todo file input and output*/
                io *in, *out;
                print_note("io.c");

                /*string input*/
                state(in = io_sin("Hello\n"));
                test(io_is_in(in));
                test(io_getc(in) == 'H');
                test(io_getc(in) == 'e');
                test(io_getc(in) == 'l');
                test(io_getc(in) == 'l');
                test(io_getc(in) == 'o');
                test(io_getc(in) == '\n');
                test(io_getc(in) == EOF);
                test(io_getc(in) == EOF);
                test(!io_error(in));
                test(io_seek(in, 0, SEEK_SET) >= 0);
                test(io_getc(in) == 'H');
                test(io_seek(in, 3, SEEK_SET) >= 0);
                test(io_getc(in) == 'l');
                test(io_ungetc('x', in) == 'x');
                test(io_getc(in) == 'x');
                test(io_getc(in) == 'o');
                state(io_close(in));

                /*string output*/
                char *s = NULL;
                state(in = io_sin("Hello,\n\tWorld!\n"));
                test(!strcmp(s = io_getline(in), "Hello,"));
                s = (free(s), NULL);
                test(!strcmp(s = io_getline(in), "\tWorld!"));
                s = (free(s), NULL);
                test(!io_getline(in));
                test(io_seek(in, 0, SEEK_SET) >= 0);
                test(!strcmp(s = io_getdelim(in, EOF), "Hello,\n\tWorld!\n"));
                s = (free(s), NULL);
                state(io_close(in));

                state(out = io_sout(calloc(1,1),1));
                test(io_puts("Hello, World", out) != EOF);
                test(!strcmp("Hello, World", io_get_string(out)));
                test(io_putc('\n', out) != EOF);
                test(!strcmp("Hello, World\n", io_get_string(out)));
                test(io_seek(out, -6, SEEK_CUR) >= 0);
                test(io_puts("Mars\n", out) != EOF);
                test(!strcmp("Hello, Mars\n\n", io_get_string(out)));
                free(io_get_string(out));
                state(io_close(out));

        }

        { /* hash.c hash table tests */
                hashtable *h = NULL;
                print_note("hash.c");
                state(h = hash_create(1));
                return_if(!h);
                test(!hash_insert(h, "key1", "val1"));
                test(!hash_insert(h, "key2", "val2"));
                /* assuming the hash algorithm is djb2, then
                 *  "heliotropes"  collides with "neurospora"
                 *  "depravement"  collides with "serafins"
                 *  "playwright"   collides with "snush" (for djb2a)
                 * See:
                 * <https://programmers.stackexchange.com/questions/49550/which-hashing-algorithm-is-best-for-uniqueness-and-speed> */
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
                test(!hash_get_load_factor(h) <= 0.75f);

                state(hash_destroy(h));
        }

        { /* lisp.c (and the lisp interpreter in general) */
                lisp *l;

                print_note("lisp.c");
                /*while unit testing eschews state being held across tests it is makes
                 *little sense in this case*/
                state(l = lisp_init()); 
                state(io_close(lisp_get_logging(l)));
                test(!lisp_set_logging(l, io_nout()));
                return_if(!l);
                test(!lisp_eval_string(l, ""));
                test(is_int(lisp_eval_string(l, "2")));
                test(get_int(lisp_eval_string(l, "(+ 2 2)")) == 4);
                test(get_int(lisp_eval_string(l, "(* 3 2)")) == 6);

                cell *x = NULL, *y = NULL, *z = NULL;
                char *t = NULL;
                state(x = intern(l, lstrdup("foo")));
                state(y = intern(l, t = lstrdup("foo"))); /*this one needs freeing!*/
                state(z = intern(l, lstrdup("bar")));
                test(x == y && x != NULL);
                test(x != z);
                free(t); /*free the non-interned string*/

                test(is_proc(lisp_eval_string(l, "(define square (lambda (x) (* x x)))")));
                test(get_int(lisp_eval_string(l, "(square 4)")) == 16);

                test(!is_list(cons(l, gsym_tee(), gsym_tee())));
                test(is_list(cons(l,  gsym_tee(), gsym_nil())));
                test(!is_list(cons(l, gsym_nil(), cons(l, gsym_tee(), gsym_tee()))));
                test(is_list(mk_list(l, gsym_tee(), gsym_nil(), gsym_tee(), NULL)));

                test(gsym_error() == lisp_eval_string(l, "(> 'a 1)"));
                test(is_sym(x));
                test(is_asciiz(x));
                test(!is_str(x));
                test(gsym_error() == lisp_eval_string(l, "(eval (cons quote 0))"));

                state(lisp_destroy(l));
        }
        return unit_test_end("liblisp"); /*should be zero!*/
}

