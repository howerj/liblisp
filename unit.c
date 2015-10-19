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
 *            vector, printing out the time taken and more!
 **/
#include "liblisp.h"

#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

/*** very minimal test framework ***/

static unsigned passed, failed;

#define test(EXPR) do {\
        if(!(EXPR)) printf("  FAILED:\t(" #EXPR ") on line '%d'\n", __LINE__), failed++;\
        else        printf("      ok:\t(" #EXPR ")\n"), passed++;\
        } while(0);

/*** end minimal test framework ***/

int main(void) {
        printf("begin liblisp unit tests\n\n");

        { 
                bitfield *b;
                printf("util.c\n");

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

                bit_set(b, 1023);
                bit_set(b, 37);
                bit_toggle(b, 37);
                bit_set(b, 0);
                bit_unset(b, 0);

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

                /* match, lstrdup, regex_match, djb2, lstrcatend, vstrcatsep,
                 * ipow, xorshift128plus
                 */ 
        }

        { /* lisp.c (and the lisp interpreter in general) */
                printf("lisp.c\n");
                /*while unit testing eschews state being held across tests it is makes
                 *little sense in this case*/
                lisp *l;
                cell *x;
                test(l = lisp_init()); 
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

                lisp_destroy(l);
        }

        printf("\npassed %u/%u\n", passed, passed+failed);
        return failed; /*should be zero!*/
}

